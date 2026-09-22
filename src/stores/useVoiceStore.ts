import { create } from 'zustand';
import { VoiceState } from '@/types';
import { eventBus } from '@/services/runtime/eventBus';
import { speechService } from '@/services/voice/speechService';
import { vaniApiClient } from '@/services/runtime/vaniApiClient';
import { useVaniStore } from '@/stores/useVaniStore';
import { conversationEngine } from '@/services/ai/conversationEngine';

interface VoiceStoreState {
  state: VoiceState;
  isMicActive: boolean;
  rawTranscript: string;
  normalizedIntent: string;
  audioLevels: number[];
  hinglishMode: boolean;
  selectedLanguage: 'English' | 'Hinglish' | 'Hindi';
  volume: number;

  // Actions
  setState: (state: VoiceState) => void;
  setMicActive: (active: boolean) => void;
  setTranscript: (raw: string, normalized?: string) => void;
  setAudioLevels: (levels: number[]) => void;
  setHinglishMode: (enabled: boolean) => void;
  setSelectedLanguage: (lang: 'English' | 'Hinglish' | 'Hindi') => void;
  triggerVoicePrompt: (promptText: string) => void;
  processCompletedTranscript: (transcript: string) => Promise<void>;
  interrupt: () => void;
  reset: () => void;
}

export const useVoiceStore = create<VoiceStoreState>((set, get) => {
  // Listen for bus events
  eventBus.on<VoiceState>('voice:state:changed', (newState) => {
    set({ state: newState });
  });

  // Listen for live speech transcript updates
  eventBus.on<{ transcript: string; isFinal: boolean }>('speech:transcript:updated', ({ transcript }) => {
    if (transcript && transcript.trim().length > 0) {
      set({ rawTranscript: transcript });
    }
  });

  // Automatically triggered when user stops speaking for 1.4 seconds after real speech
  eventBus.on<{ transcript: string }>('speech:silence:completed', ({ transcript }) => {
    if (transcript && transcript.trim().length > 1 && transcript !== 'Listening...') {
      get().processCompletedTranscript(transcript.trim());
    }
  });

  eventBus.on<{ transcript?: string }>('speech:listening:ended', ({ transcript }) => {
    const text = transcript ? transcript.trim() : '';
    if (text && text.length > 1 && text !== 'Listening...' && get().state === 'listening') {
      get().processCompletedTranscript(text);
    } else if (get().state === 'listening') {
      set({ state: 'idle', isMicActive: false });
    }
  });

  return {
    state: 'idle',
    isMicActive: false,
    rawTranscript: '',
    normalizedIntent: '',
    audioLevels: [20, 45, 80, 55, 30, 60, 40, 75, 50, 30, 20],
    hinglishMode: true,
    selectedLanguage: 'Hinglish',
    volume: 85,

    setState: (state) => set({ state }),

    setMicActive: (isMicActive) => {
      set({ isMicActive, state: isMicActive ? 'listening' : 'idle' });
      if (isMicActive) {
        // Clear all previous state and buffers completely
        set({ rawTranscript: '', normalizedIntent: '' });
        const lang = get().selectedLanguage;
        const started = speechService.startListening(lang);
        if (!started) {
          console.warn('[Voice] Native speech recognition not started');
        }
      } else {
        speechService.stopListening();
        speechService.stopSpeaking();
      }
    },

    setTranscript: (rawTranscript, normalizedIntent = '') => set({ rawTranscript, normalizedIntent }),
    setAudioLevels: (audioLevels) => set({ audioLevels }),
    setHinglishMode: (hinglishMode) => set({ hinglishMode }),
    setSelectedLanguage: (selectedLanguage) => set({ selectedLanguage }),

    processCompletedTranscript: async (transcriptText: string) => {
      const clean = transcriptText.trim();
      if (!clean || clean.length <= 1 || clean === 'Listening...' || clean === 'Interrupted by user.') {
        return;
      }

      speechService.stopListening();
      set({ state: 'thinking', rawTranscript: clean, isMicActive: false });

      // Generate intelligent conversational AI response
      const vaniState = useVaniStore.getState();
      const aiResponse = await conversationEngine.generateResponse(clean, {
        activeTasksCount: vaniState.tasks.filter((t) => t.state === 'running').length,
        cpuUsage: vaniState.systemMetrics?.cpuUsage ?? 18,
        memoryUsage: vaniState.systemMetrics ? Math.round((vaniState.systemMetrics.ramUsedGb / vaniState.systemMetrics.ramTotalGb) * 100) : 42,
        isOffline: vaniState.isOffline,
        activeModel: vaniState.models.find((m) => m.status === 'active')?.name || 'Llama 3.3 70B (Local)',
      });

      set({
        state: 'executing',
        normalizedIntent: aiResponse.understoodIntent || clean,
      });

      // Post into Chat System
      useVaniStore.getState().sendChatMessage(clean, false);

      eventBus.emit('activity:created', {
        id: `act-${Date.now()}`,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
        action: `Voice Command: "${clean}"`,
        target: aiResponse.understoodIntent || clean,
        type: 'voice',
      });

      // Post to Backend Gateway API
      vaniApiClient.processVoice(clean).catch(() => {});

      // Talk back aloud with natural synthesized voice
      setTimeout(async () => {
        set({ state: 'speaking' });
        await speechService.speak(aiResponse.speechText);
        set({ state: 'idle', isMicActive: false });
      }, 300);
    },

    triggerVoicePrompt: (promptText) => {
      set({ state: 'listening', isMicActive: true, rawTranscript: promptText });
      get().processCompletedTranscript(promptText);
    },

    interrupt: () => {
      speechService.stopListening();
      speechService.stopSpeaking();
      set({ state: 'idle', isMicActive: false, rawTranscript: '', normalizedIntent: '' });
      eventBus.emit('activity:created', {
        id: `act-${Date.now()}`,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
        action: 'Voice generation interrupted',
        target: 'VANI Core',
        type: 'voice',
      });
    },

    reset: () => {
      speechService.stopListening();
      speechService.stopSpeaking();
      set({ state: 'idle', isMicActive: false, rawTranscript: '', normalizedIntent: '' });
    },
  };
});
