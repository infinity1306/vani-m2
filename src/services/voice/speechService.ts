/**
 * VANI Mark 2 — Native Web Speech API Bridge
 * Real-time Speech-to-Text (STT) & Text-to-Speech (TTS) Talkback
 */
import { eventBus } from '../runtime/eventBus';
import { VoiceState } from '@/types';

type SpeechRecognitionType = any;

class SpeechService {
  private recognition: SpeechRecognitionType | null = null;
  private isListening: boolean = false;
  private isSpeaking: boolean = false;
  private preferredVoice: SpeechSynthesisVoice | null = null;
  private silenceTimer: NodeJS.Timeout | null = null;
  private currentSessionTranscript: string = '';
  private hasRecognizedSpeechInSession: boolean = false;

  constructor() {
    this.initRecognition();
    this.initVoices();
  }

  private initVoices() {
    if (typeof window === 'undefined' || !window.speechSynthesis) return;

    const loadVoices = () => {
      const voices = window.speechSynthesis.getVoices();
      const indianVoice = voices.find(
        (v) => v.lang.includes('en-IN') || v.name.includes('India') || v.lang.includes('hi-IN')
      );
      const googleVoice = voices.find(
        (v) => v.name.includes('Google') || v.name.includes('Natural') || v.name.includes('David') || v.name.includes('Zira')
      );
      this.preferredVoice = indianVoice || googleVoice || voices[0] || null;
    };

    loadVoices();
    if (window.speechSynthesis.onvoiceschanged !== undefined) {
      window.speechSynthesis.onvoiceschanged = loadVoices;
    }
  }

  private initRecognition() {
    if (typeof window === 'undefined') return;

    const SpeechRecognition =
      (window as any).SpeechRecognition || (window as any).webkitSpeechRecognition;

    if (!SpeechRecognition) {
      console.warn('[SpeechService] Web Speech Recognition API not supported in this browser.');
      return;
    }

    try {
      this.recognition = new SpeechRecognition();
      this.recognition.continuous = true;
      this.recognition.interimResults = true;
      this.recognition.lang = 'en-IN';

      this.recognition.onstart = () => {
        this.isListening = true;
        this.currentSessionTranscript = '';
        this.hasRecognizedSpeechInSession = false;
        eventBus.emit('voice:state:changed', 'listening' as VoiceState);
        eventBus.emit('speech:listening:started', {});
      };

      this.recognition.onresult = (event: any) => {
        let interimTranscript = '';
        let finalTranscript = '';

        for (let i = event.resultIndex; i < event.results.length; ++i) {
          const transcript = event.results[i][0].transcript;
          if (event.results[i].isFinal) {
            finalTranscript += transcript;
          } else {
            interimTranscript += transcript;
          }
        }

        const currentText = (finalTranscript || interimTranscript).trim();
        if (currentText && currentText.length > 0) {
          this.currentSessionTranscript = currentText;
          this.hasRecognizedSpeechInSession = true;

          eventBus.emit('speech:transcript:updated', {
            transcript: currentText,
            isFinal: !!finalTranscript,
          });

          // Reset silence timer on every recognized word
          if (this.silenceTimer) {
            clearTimeout(this.silenceTimer);
          }

          // Trigger completion only when user speaks and then pauses for 1.4 seconds
          this.silenceTimer = setTimeout(() => {
            if (this.hasRecognizedSpeechInSession && this.currentSessionTranscript.trim().length > 1) {
              const textToSend = this.currentSessionTranscript;
              this.stopListening();
              eventBus.emit('speech:silence:completed', { transcript: textToSend });
            }
          }, 1400);
        }
      };

      this.recognition.onerror = (event: any) => {
        console.warn('[SpeechService] Speech recognition notice:', event.error);
        if (event.error === 'not-allowed') {
          alert('Please allow Microphone access in your browser to talk with VANI.');
          this.isListening = false;
          eventBus.emit('voice:state:changed', 'idle' as VoiceState);
        } else if (event.error !== 'no-speech') {
          this.isListening = false;
          eventBus.emit('voice:state:changed', 'idle' as VoiceState);
        }
      };

      this.recognition.onend = () => {
        this.isListening = false;
        if (this.silenceTimer) {
          clearTimeout(this.silenceTimer);
          this.silenceTimer = null;
        }

        // Only emit ended with transcript if actual words were spoken
        if (this.hasRecognizedSpeechInSession && this.currentSessionTranscript.trim().length > 1) {
          eventBus.emit('speech:listening:ended', { transcript: this.currentSessionTranscript });
        } else {
          eventBus.emit('speech:listening:ended', { transcript: '' });
        }
      };
    } catch (err) {
      console.error('[SpeechService] Initialization error:', err);
    }
  }

  // Start capturing from microphone
  startListening(lang: string = 'en-IN'): boolean {
    this.stopSpeaking();
    if (this.silenceTimer) {
      clearTimeout(this.silenceTimer);
      this.silenceTimer = null;
    }
    this.currentSessionTranscript = '';
    this.hasRecognizedSpeechInSession = false;

    if (!this.recognition) {
      this.initRecognition();
    }

    if (!this.recognition) {
      return false;
    }

    try {
      this.recognition.lang = lang === 'Hindi' ? 'hi-IN' : 'en-IN';
      this.recognition.start();
      this.isListening = true;
      return true;
    } catch (err) {
      try {
        this.recognition.stop();
        setTimeout(() => {
          try {
            this.recognition.start();
            this.isListening = true;
          } catch (e) {}
        }, 100);
        return true;
      } catch (e) {
        console.warn('[SpeechService] startListening error:', e);
        return false;
      }
    }
  }

  // Stop capturing
  stopListening() {
    if (this.silenceTimer) {
      clearTimeout(this.silenceTimer);
      this.silenceTimer = null;
    }
    if (this.recognition) {
      try {
        this.recognition.stop();
      } catch (err) {}
    }
    this.isListening = false;
  }

  // Speak response back aloud using browser TTS
  speak(
    text: string,
    onStart?: () => void,
    onEnd?: () => void
  ): Promise<void> {
    return new Promise((resolve) => {
      if (typeof window === 'undefined' || !window.speechSynthesis) {
        if (onEnd) onEnd();
        resolve();
        return;
      }

      this.stopSpeaking();

      // Clean markdown, symbols, hashes
      const cleanText = text
        .replace(/[*#_`~[\]()]/g, '')
        .replace(/https?:\/\/\S+/g, 'link')
        .trim();

      if (!cleanText) {
        if (onEnd) onEnd();
        resolve();
        return;
      }

      if (window.speechSynthesis.paused) {
        window.speechSynthesis.resume();
      }

      const utterance = new SpeechSynthesisUtterance(cleanText);
      utterance.rate = 1.0;
      utterance.pitch = 1.0;
      utterance.volume = 1.0;

      if (this.preferredVoice) {
        utterance.voice = this.preferredVoice;
      }

      utterance.onstart = () => {
        this.isSpeaking = true;
        eventBus.emit('voice:state:changed', 'speaking' as VoiceState);
        if (onStart) onStart();
      };

      utterance.onend = () => {
        this.isSpeaking = false;
        eventBus.emit('voice:state:changed', 'idle' as VoiceState);
        if (onEnd) onEnd();
        resolve();
      };

      utterance.onerror = (e) => {
        console.warn('[SpeechService] TTS playback error/interrupt:', e);
        this.isSpeaking = false;
        eventBus.emit('voice:state:changed', 'idle' as VoiceState);
        if (onEnd) onEnd();
        resolve();
      };

      window.speechSynthesis.speak(utterance);
    });
  }

  stopSpeaking() {
    if (typeof window !== 'undefined' && window.speechSynthesis) {
      window.speechSynthesis.cancel();
    }
    this.isSpeaking = false;
  }

  getIsListening(): boolean {
    return this.isListening;
  }

  getIsSpeaking(): boolean {
    return this.isSpeaking;
  }
}

export const speechService = new SpeechService();
