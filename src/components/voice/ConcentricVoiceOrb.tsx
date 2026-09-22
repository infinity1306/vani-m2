import React from 'react';
import { motion } from 'framer-motion';
import { VoiceState } from '@/types';
import { Mic, MicOff, Square, Sparkles, AlertCircle, ShieldAlert, Cpu } from 'lucide-react';
import { useVoiceStore } from '@/stores/useVoiceStore';

interface ConcentricVoiceOrbProps {
  state?: VoiceState;
  size?: 'sm' | 'md' | 'lg' | 'hero';
  showControls?: boolean;
  onOrbClick?: () => void;
}

export const ConcentricVoiceOrb: React.FC<ConcentricVoiceOrbProps> = ({
  state: propState,
  size = 'hero',
  showControls = true,
  onOrbClick,
}) => {
  const storeState = useVoiceStore((s) => s.state);
  const isMicActive = useVoiceStore((s) => s.isMicActive);
  const setMicActive = useVoiceStore((s) => s.setMicActive);
  const interrupt = useVoiceStore((s) => s.interrupt);
  const triggerVoicePrompt = useVoiceStore((s) => s.triggerVoicePrompt);

  const state = propState || storeState;

  const sizeDimensions = {
    sm: { container: 'w-16 h-16', rings: 'w-14 h-14', center: 'w-8 h-8', icon: 'w-4 h-4' },
    md: { container: 'w-28 h-28', rings: 'w-24 h-24', center: 'w-12 h-12', icon: 'w-5 h-5' },
    lg: { container: 'w-44 h-44', rings: 'w-36 h-36', center: 'w-16 h-16', icon: 'w-6 h-6' },
    hero: { container: 'w-64 h-64 md:w-80 md:h-80', rings: 'w-52 h-52 md:w-64 md:h-64', center: 'w-24 h-24 md:w-28 md:h-28', icon: 'w-8 h-8' },
  };

  const getGlowColor = () => {
    switch (state) {
      case 'listening':
        return 'from-cyan-500/30 via-purple-600/30 to-indigo-600/30 shadow-[0_0_60px_rgba(6,182,212,0.3)]';
      case 'thinking':
        return 'from-purple-500/40 via-indigo-500/40 to-cyan-500/30 shadow-[0_0_70px_rgba(139,92,246,0.35)]';
      case 'executing':
        return 'from-indigo-600/40 via-purple-600/40 to-pink-500/30 shadow-[0_0_80px_rgba(99,102,241,0.4)]';
      case 'speaking':
        return 'from-cyan-400/40 via-purple-500/50 to-indigo-500/40 shadow-[0_0_90px_rgba(168,85,247,0.45)]';
      case 'waiting_confirmation':
        return 'from-amber-500/40 via-orange-600/40 to-yellow-500/30 shadow-[0_0_60px_rgba(245,158,11,0.35)]';
      case 'error':
        return 'from-rose-600/40 via-red-500/40 to-rose-700/30 shadow-[0_0_60px_rgba(244,63,94,0.35)]';
      case 'offline':
        return 'from-slate-700/30 via-purple-950/20 to-slate-800/30 shadow-[0_0_40px_rgba(100,116,139,0.2)]';
      default: // idle
        return 'from-purple-900/30 via-indigo-900/20 to-cyan-950/30 shadow-[0_0_50px_rgba(139,92,246,0.2)]';
    }
  };

  const getStatusLabel = () => {
    switch (state) {
      case 'listening':
        return { label: 'Listening to environment...', color: 'text-cyan-300' };
      case 'thinking':
        return { label: 'Synthesizing neural plan...', color: 'text-purple-300' };
      case 'executing':
        return { label: 'Autonomous agents executing...', color: 'text-indigo-300' };
      case 'speaking':
        return { label: 'VANI speaking...', color: 'text-purple-200' };
      case 'waiting_confirmation':
        return { label: 'Waiting for permission...', color: 'text-amber-300' };
      case 'error':
        return { label: 'Subsystem anomaly detected', color: 'text-rose-300' };
      case 'offline':
        return { label: 'Local-only mode active', color: 'text-slate-400' };
      default:
        return { label: 'VANI Ready • Press space or click to speak', color: 'text-slate-400' };
    }
  };

  const status = getStatusLabel();

  const handleToggle = () => {
    if (onOrbClick) {
      onOrbClick();
      return;
    }
    if (state === 'speaking' || state === 'executing') {
      interrupt();
    } else if (state === 'listening') {
      setMicActive(false);
    } else {
      triggerVoicePrompt('mera react wala project run karke auth check karde');
    }
  };

  return (
    <div className="flex flex-col items-center justify-center select-none">
      {/* Orb Canvas Wrapper */}
      <div
        onClick={handleToggle}
        className={`relative ${sizeDimensions[size].container} flex items-center justify-center cursor-pointer group`}
      >
        {/* Ambient background glow aura */}
        <div
          className={`absolute inset-0 rounded-full bg-gradient-to-tr ${getGlowColor()} filter blur-2xl transition-all duration-700 opacity-80 group-hover:opacity-100`}
        />

        {/* Outer concentric pulse ring 1 */}
        <motion.div
          animate={
            state === 'listening' || state === 'speaking'
              ? { scale: [1, 1.15, 1], opacity: [0.3, 0.7, 0.3] }
              : state === 'thinking'
              ? { rotate: 360, scale: [1, 1.05, 1] }
              : { scale: [1, 1.04, 1], opacity: [0.2, 0.4, 0.2] }
          }
          transition={{
            duration: state === 'thinking' ? 6 : 3,
            repeat: Infinity,
            ease: 'easeInOut',
          }}
          className={`absolute inset-0 rounded-full border border-purple-500/20 border-dashed`}
        />

        {/* Outer concentric ring 2 */}
        <motion.div
          animate={
            state === 'thinking'
              ? { rotate: -360 }
              : state === 'executing'
              ? { scale: [1, 1.08, 1], rotate: 180 }
              : { rotate: 180 }
          }
          transition={{ duration: 18, repeat: Infinity, ease: 'linear' }}
          className={`absolute ${sizeDimensions[size].rings} rounded-full border border-cyan-500/25`}
        >
          {/* Orbital node dots */}
          <div className="absolute top-0 left-1/2 -translate-x-1/2 -translate-y-1/2 w-2 h-2 rounded-full bg-cyan-400 shadow-[0_0_8px_#22d3ee]" />
          <div className="absolute bottom-0 left-1/2 -translate-x-1/2 translate-y-1/2 w-1.5 h-1.5 rounded-full bg-purple-400 shadow-[0_0_8px_#c084fc]" />
        </motion.div>

        {/* Inner concentric ring 3 */}
        <motion.div
          animate={
            state === 'listening'
              ? { scale: [0.95, 1.1, 0.95] }
              : state === 'speaking'
              ? { scale: [0.9, 1.15, 0.9] }
              : { scale: 1 }
          }
          transition={{ duration: 1.4, repeat: Infinity, ease: 'easeInOut' }}
          className={`absolute ${sizeDimensions[size].rings} rounded-full border border-purple-400/40`}
        />

        {/* Central Core Sphere / Waveform container */}
        <motion.div
          whileHover={{ scale: 1.05 }}
          whileTap={{ scale: 0.95 }}
          className={`relative ${sizeDimensions[size].center} rounded-full bg-gradient-to-b from-slate-900 via-[#101426] to-[#0a0d18] border border-white/20 shadow-2xl flex items-center justify-center overflow-hidden z-10`}
        >
          {/* Animated Central Waveform Bars */}
          <div className="flex items-center justify-center gap-1 h-12 px-2">
            {state === 'speaking' ? (
              // Vibrant animated speech waveform
              [16, 28, 44, 32, 48, 36, 20].map((height, i) => (
                <motion.div
                  key={i}
                  animate={{ height: [8, height, 12, height * 0.8, 8] }}
                  transition={{ duration: 0.8 + i * 0.1, repeat: Infinity, ease: 'easeInOut' }}
                  className="w-1 md:w-1.5 bg-gradient-to-t from-purple-500 via-cyan-400 to-white rounded-full shadow-[0_0_6px_#a855f7]"
                />
              ))
            ) : state === 'listening' ? (
              // Listening frequency ripple
              [12, 24, 38, 24, 12].map((height, i) => (
                <motion.div
                  key={i}
                  animate={{ height: [6, height, 6] }}
                  transition={{ duration: 0.6 + i * 0.1, repeat: Infinity, ease: 'easeInOut' }}
                  className="w-1 md:w-1.5 bg-gradient-to-t from-cyan-500 to-cyan-200 rounded-full shadow-[0_0_6px_#06b6d4]"
                />
              ))
            ) : state === 'thinking' ? (
              // Thinking triple orbital pulse
              <div className="flex gap-1.5">
                {[0, 1, 2].map((i) => (
                  <motion.div
                    key={i}
                    animate={{ scale: [0.8, 1.4, 0.8], opacity: [0.4, 1, 0.4] }}
                    transition={{ duration: 1, delay: i * 0.2, repeat: Infinity }}
                    className="w-2 h-2 rounded-full bg-purple-400 shadow-[0_0_8px_#c084fc]"
                  />
                ))}
              </div>
            ) : state === 'waiting_confirmation' ? (
              <ShieldAlert className="w-7 h-7 text-amber-400 animate-pulse" />
            ) : state === 'error' ? (
              <AlertCircle className="w-7 h-7 text-rose-400 animate-bounce" />
            ) : state === 'offline' ? (
              <Cpu className="w-6 h-6 text-slate-400" />
            ) : (
              // Idle state - calm concentric glow icon
              <Sparkles className="w-6 h-6 text-purple-300 opacity-80 group-hover:opacity-100 group-hover:text-purple-200 transition-colors" />
            )}
          </div>
        </motion.div>
      </div>

      {/* Status Micro-label below Orb */}
      {showControls && (
        <div className="mt-4 flex flex-col items-center gap-1.5 text-center">
          <p className={`text-xs md:text-sm font-medium tracking-wide ${status.color}`}>
            {status.label}
          </p>
        </div>
      )}
    </div>
  );
};
