import React, { useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import {
  Mic,
  MicOff,
  Square,
  Sparkles,
  ChevronUp,
  ChevronDown,
  Languages,
  ArrowRight,
  Volume2,
  Cpu,
} from 'lucide-react';
import { useVoiceStore } from '@/stores/useVoiceStore';
import { AudioSpectrumBar } from '@/components/voice/AudioSpectrumBar';
import { Button } from '@/components/ui/Button';

export const GlobalVoiceController: React.FC = () => {
  const state = useVoiceStore((s) => s.state);
  const isMicActive = useVoiceStore((s) => s.isMicActive);
  const setMicActive = useVoiceStore((s) => s.setMicActive);
  const rawTranscript = useVoiceStore((s) => s.rawTranscript);
  const normalizedIntent = useVoiceStore((s) => s.normalizedIntent);
  const hinglishMode = useVoiceStore((s) => s.hinglishMode);
  const setHinglishMode = useVoiceStore((s) => s.setHinglishMode);
  const triggerVoicePrompt = useVoiceStore((s) => s.triggerVoicePrompt);
  const interrupt = useVoiceStore((s) => s.interrupt);

  const [isExpanded, setIsExpanded] = useState(false);

  const quickPrompts = [
    'mera react wala project run karde',
    'take screenshot and analyze UI',
    'research 2026 AI agent frameworks',
    'pull latest changes from GitHub',
  ];

  const getAuraColor = () => {
    switch (state) {
      case 'listening':
        return 'border-cyan-500/50 shadow-[0_0_30px_rgba(6,182,212,0.25)]';
      case 'thinking':
        return 'border-purple-500/50 shadow-[0_0_30px_rgba(139,92,246,0.3)]';
      case 'executing':
        return 'border-indigo-500/50 shadow-[0_0_35px_rgba(99,102,241,0.3)]';
      case 'speaking':
        return 'border-purple-400 shadow-[0_0_40px_rgba(168,85,247,0.35)]';
      default:
        return 'border-white/10 hover:border-purple-500/30';
    }
  };

  return (
    <div className="fixed bottom-4 right-4 md:right-8 z-50 select-none">
      <motion.div
        layout
        className={`bg-[#0c111e]/90 backdrop-blur-xl border ${getAuraColor()} rounded-2xl shadow-2xl transition-all duration-300 overflow-hidden ${
          isExpanded ? 'w-80 md:w-96' : 'w-auto'
        }`}
      >
        {/* Main Dock Header Bar */}
        <div className="flex items-center justify-between p-2.5 md:p-3 gap-3">
          {/* Mic / Core Button */}
          <button
            onClick={() => {
              if (state === 'speaking' || state === 'executing') {
                interrupt();
              } else {
                setMicActive(!isMicActive);
              }
            }}
            className={`w-10 h-10 rounded-xl flex items-center justify-center transition-all ${
              isMicActive || state !== 'idle'
                ? 'bg-gradient-to-tr from-purple-600 to-cyan-500 text-white shadow-[0_0_15px_#8b5cf6]'
                : 'bg-white/[0.06] text-slate-300 hover:bg-white/[0.1]'
            }`}
          >
            {state === 'speaking' || state === 'executing' ? (
              <Square className="w-4 h-4 fill-white" />
            ) : isMicActive ? (
              <Mic className="w-4 h-4 animate-bounce" />
            ) : (
              <Mic className="w-4 h-4" />
            )}
          </button>

          {/* Voice Waveform Indicator */}
          <div
            onClick={() => setIsExpanded(!isExpanded)}
            className="flex-1 min-w-0 flex flex-col justify-center cursor-pointer px-1"
          >
            <div className="flex items-center justify-between">
              <span className="text-xs font-semibold text-white truncate">
                {state === 'idle' ? 'VANI Voice Layer' : state.toUpperCase()}
              </span>
              <span className="text-[10px] font-mono text-purple-400">
                {hinglishMode ? 'Hinglish' : 'EN'}
              </span>
            </div>
            <div className="mt-1">
              <AudioSpectrumBar isActive={state !== 'idle'} color={state === 'listening' ? 'cyan' : 'purple'} />
            </div>
          </div>

          {/* Expand / Minimize Toggle */}
          <button
            onClick={() => setIsExpanded(!isExpanded)}
            className="p-1.5 rounded-lg text-slate-400 hover:text-white hover:bg-white/[0.06] transition-colors"
          >
            {isExpanded ? <ChevronDown className="w-4 h-4" /> : <ChevronUp className="w-4 h-4" />}
          </button>
        </div>

        {/* Expanded Intelligence & Transcript Panel */}
        <AnimatePresence>
          {isExpanded && (
            <motion.div
              initial={{ height: 0, opacity: 0 }}
              animate={{ height: 'auto', opacity: 1 }}
              exit={{ height: 0, opacity: 0 }}
              className="px-4 pb-4 pt-1 border-t border-white/10 space-y-3"
            >
              {/* Transcript box with Hinglish transparency */}
              <div className="p-3 rounded-xl bg-black/40 border border-white/5 space-y-2">
                <div className="flex items-center justify-between text-[11px] text-slate-400 font-mono">
                  <span>Live Acoustic Buffer</span>
                  <button
                    onClick={() => setHinglishMode(!hinglishMode)}
                    className="flex items-center gap-1 text-purple-400 hover:text-purple-300"
                  >
                    <Languages className="w-3 h-3" />
                    <span>{hinglishMode ? 'Hinglish Active' : 'English Only'}</span>
                  </button>
                </div>

                <p className={`text-xs leading-relaxed font-medium ${
                  isMicActive && !rawTranscript ? 'text-cyan-400 animate-pulse' : 'text-slate-200'
                }`}>
                  {isMicActive
                    ? rawTranscript
                      ? `"${rawTranscript}"`
                      : '🎤 Listening to your microphone... Please speak now!'
                    : rawTranscript || 'Say "VANI" or click mic to talk...'}
                </p>

                {normalizedIntent && (
                  <div className="pt-2 border-t border-white/5">
                    <span className="text-[10px] uppercase font-mono text-cyan-400">Understood as:</span>
                    <p className="text-xs text-cyan-200 mt-0.5">{normalizedIntent}</p>
                  </div>
                )}
              </div>

              {/* Manual Input Fallback & Action Sender */}
              <div className="flex items-center gap-2 pt-1">
                <input
                  type="text"
                  placeholder="Or type a question/command..."
                  onKeyDown={(e) => {
                    if (e.key === 'Enter' && e.currentTarget.value.trim()) {
                      triggerVoicePrompt(e.currentTarget.value.trim());
                      e.currentTarget.value = '';
                    }
                  }}
                  className="flex-1 px-3 py-1.5 rounded-xl bg-white/[0.04] border border-white/10 text-xs text-slate-200 placeholder:text-slate-500 focus:outline-none focus:border-purple-500/50"
                />
              </div>

              {/* Quick Prompt Chips */}
              <div>
                <span className="text-[10px] uppercase font-mono text-slate-400 block mb-1.5">
                  Try speaking or click:
                </span>
                <div className="flex flex-wrap gap-1.5">
                  {quickPrompts.map((p, i) => (
                    <button
                      key={i}
                      onClick={() => triggerVoicePrompt(p)}
                      className="text-[11px] px-2.5 py-1 rounded-lg bg-white/[0.04] hover:bg-purple-600/20 border border-white/5 hover:border-purple-500/30 text-slate-300 hover:text-white transition-all text-left truncate max-w-[200px]"
                    >
                      {p}
                    </button>
                  ))}
                </div>
              </div>
            </motion.div>
          )}
        </AnimatePresence>
      </motion.div>
    </div>
  );
};
