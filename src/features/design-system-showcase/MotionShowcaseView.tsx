import React, { useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import {
  Sparkles,
  Mic,
  Bot,
  CheckSquare,
  Cpu,
  ShieldAlert,
  Bell,
  Layers,
  ArrowRight,
  Shield,
  Activity,
  Maximize2,
} from 'lucide-react';
import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { Skeleton } from '@/components/ui/Skeleton';
import { Modal } from '@/components/ui/Modal';
import { Drawer } from '@/components/ui/Drawer';
import { ConcentricVoiceOrb } from '@/components/voice/ConcentricVoiceOrb';
import { AudioSpectrumBar } from '@/components/voice/AudioSpectrumBar';
import { slideUpFade, fadeInScale } from '@/design-system/motion';

export const MotionShowcaseView: React.FC = () => {
  const [demoProgress, setDemoProgress] = useState(60);
  const [isModalOpen, setIsModalOpen] = useState(false);
  const [isDrawerOpen, setIsDrawerOpen] = useState(false);
  const [showToast, setShowToast] = useState(false);

  const triggerToast = () => {
    setShowToast(true);
    setTimeout(() => setShowToast(false), 3000);
  };

  return (
    <motion.div variants={slideUpFade} initial="initial" animate="animate" className="space-y-8">
      {/* Header */}
      <div>
        <h1 className="text-xl md:text-2xl font-bold text-white tracking-tight">
          Motion & Transitions Showcase
        </h1>
        <p className="text-xs md:text-sm text-slate-400">
          Design system source of truth for micro-interactions, voice auras, and card physics.
        </p>
      </div>

      {/* 1. Feature Cards Showcase (Prompt #12 & Image top-right) */}
      <div className="space-y-4">
        <h3 className="text-xs uppercase font-mono text-slate-400 font-semibold tracking-wider">
          1. Feature Card Family & Micro-States
        </h3>

        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
          {/* Voice Listening Card */}
          <Card className="space-y-3">
            <div className="flex items-center justify-between text-xs">
              <span className="text-slate-400 font-mono">Voice State</span>
              <Badge variant="cyan" size="xs" dot>Listening</Badge>
            </div>
            <div className="flex justify-center py-2">
              <ConcentricVoiceOrb state="listening" size="sm" showControls={false} />
            </div>
            <p className="text-[11px] text-center text-cyan-300 font-mono">Acoustic Ripple Active</p>
          </Card>

          {/* Voice Thinking Card */}
          <Card className="space-y-3">
            <div className="flex items-center justify-between text-xs">
              <span className="text-slate-400 font-mono">Voice State</span>
              <Badge variant="primary" size="xs" dot>Thinking</Badge>
            </div>
            <div className="flex justify-center py-2">
              <ConcentricVoiceOrb state="thinking" size="sm" showControls={false} />
            </div>
            <p className="text-[11px] text-center text-purple-300 font-mono">Orbital Synthesis</p>
          </Card>

          {/* Voice Speaking Card */}
          <Card className="space-y-3">
            <div className="flex items-center justify-between text-xs">
              <span className="text-slate-400 font-mono">Voice State</span>
              <Badge variant="primary" size="xs" dot>Speaking</Badge>
            </div>
            <div className="flex justify-center py-2">
              <ConcentricVoiceOrb state="speaking" size="sm" showControls={false} />
            </div>
            <p className="text-[11px] text-center text-purple-200 font-mono">Multi-band Waveform</p>
          </Card>

          {/* Permission Alert Card */}
          <Card variant="warning" className="space-y-3 bg-amber-950/20 border-amber-500/40">
            <div className="flex items-center justify-between text-xs">
              <span className="text-amber-300 font-mono">Elevation</span>
              <Badge variant="warning" size="xs">Required</Badge>
            </div>
            <div className="text-center py-1">
              <ShieldAlert className="w-8 h-8 text-amber-400 mx-auto animate-pulse" />
              <p className="text-xs font-semibold text-white mt-1">Terminal Execution</p>
            </div>
            <div className="flex gap-1.5 pt-1">
              <Button variant="danger" size="xs" className="w-1/2">Deny</Button>
              <Button variant="primary" size="xs" className="w-1/2">Allow</Button>
            </div>
          </Card>

          {/* Task Progress Card */}
          <Card className="space-y-3">
            <div className="flex justify-between text-xs">
              <span className="font-semibold text-white">Research AI Tools</span>
              <span className="text-purple-400 font-mono font-bold">{demoProgress}%</span>
            </div>
            <div className="w-full h-1.5 bg-white/10 rounded-full overflow-hidden">
              <motion.div
                animate={{ width: `${demoProgress}%` }}
                className="h-full bg-gradient-to-r from-purple-500 to-cyan-400 rounded-full"
              />
            </div>
            <div className="flex gap-2">
              <Button variant="secondary" size="xs" onClick={() => setDemoProgress(Math.min(100, demoProgress + 15))}>
                +15%
              </Button>
              <Button variant="ghost" size="xs" onClick={() => setDemoProgress(20)}>
                Reset
              </Button>
            </div>
          </Card>

          {/* Agent Running Card */}
          <Card className="space-y-3">
            <div className="flex items-center justify-between text-xs">
              <div className="flex items-center gap-2">
                <span className="text-base">⚡</span>
                <span className="font-semibold text-white">Odysseus</span>
              </div>
              <Badge variant="primary" size="xs" dot>Running</Badge>
            </div>
            <p className="text-xs text-slate-300">AST Code Refactoring on React Auth module</p>
            <div className="flex items-center gap-1 text-[10px] text-purple-300 font-mono">
              <span>Tools:</span>
              <span className="px-1 bg-white/5 rounded">Git</span>
              <span className="px-1 bg-white/5 rounded">Terminal</span>
            </div>
          </Card>

          {/* Local Model Card */}
          <Card className="space-y-3">
            <div className="flex justify-between text-xs">
              <span className="text-slate-400 font-mono">Local Engine</span>
              <Badge variant="success" size="xs" dot>Active</Badge>
            </div>
            <div>
              <h4 className="text-sm font-bold text-white">Llama 3.8B</h4>
              <p className="text-xs text-purple-300 font-mono">32ms TTFT • Ollama</p>
            </div>
            <div className="w-full h-1 bg-white/10 rounded-full overflow-hidden">
              <div className="w-1/4 h-full bg-purple-400" />
            </div>
          </Card>

          {/* Cloud Model Card */}
          <Card className="space-y-3">
            <div className="flex justify-between text-xs">
              <span className="text-slate-400 font-mono">Cloud Mesh</span>
              <Badge variant="cyan" size="xs" dot>Online</Badge>
            </div>
            <div>
              <h4 className="text-sm font-bold text-white">Gemini 1.5 Pro</h4>
              <p className="text-xs text-cyan-300 font-mono">1M Context • Google Cloud</p>
            </div>
            <div className="w-full h-1 bg-white/10 rounded-full overflow-hidden">
              <div className="w-3/4 h-full bg-cyan-400" />
            </div>
          </Card>
        </div>
      </div>

      {/* 2. Interactive Animation Showcase Pipeline (Image bottom pipeline) */}
      <div className="space-y-4">
        <h3 className="text-xs uppercase font-mono text-slate-400 font-semibold tracking-wider">
          2. Transition Pipeline & Interactive Triggers
        </h3>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
          {/* Modal Trigger */}
          <Card variant="interactive" onClick={() => setIsModalOpen(true)} className="space-y-2">
            <div className="flex items-center justify-between">
              <h4 className="text-sm font-semibold text-white">Modal Transition</h4>
              <Maximize2 className="w-4 h-4 text-purple-400" />
            </div>
            <p className="text-xs text-slate-400">Scale + Fade-in with backdrop blur spring</p>
            <Button variant="secondary" size="xs" className="mt-2">
              Launch Modal (Scale + Fade)
            </Button>
          </Card>

          {/* Drawer Trigger */}
          <Card variant="interactive" onClick={() => setIsDrawerOpen(true)} className="space-y-2">
            <div className="flex items-center justify-between">
              <h4 className="text-sm font-semibold text-white">Drawer Transition</h4>
              <ArrowRight className="w-4 h-4 text-cyan-400" />
            </div>
            <p className="text-xs text-slate-400">Right-to-left spring damped slide</p>
            <Button variant="secondary" size="xs" className="mt-2">
              Launch Drawer (Slide + Spring)
            </Button>
          </Card>

          {/* Toast Notification Trigger */}
          <Card variant="interactive" onClick={triggerToast} className="space-y-2">
            <div className="flex items-center justify-between">
              <h4 className="text-sm font-semibold text-white">Toast Notification</h4>
              <Bell className="w-4 h-4 text-emerald-400" />
            </div>
            <p className="text-xs text-slate-400">Slide-in notification with glow border</p>
            <Button variant="secondary" size="xs" className="mt-2">
              Trigger Toast Notification
            </Button>
          </Card>
        </div>
      </div>

      {/* 3. Skeleton Loading State Demo */}
      <div className="space-y-3">
        <h3 className="text-xs uppercase font-mono text-slate-400 font-semibold tracking-wider">
          3. Skeleton Loading State (No Spinners)
        </h3>
        <Card className="space-y-3">
          <div className="flex items-center gap-3">
            <Skeleton variant="circular" className="w-10 h-10" />
            <div className="space-y-1.5 flex-1">
              <Skeleton variant="text" className="w-1/3 h-4" />
              <Skeleton variant="text" className="w-1/4 h-3" />
            </div>
          </div>
          <Skeleton variant="rectangular" className="w-full h-12" />
        </Card>
      </div>

      {/* Modals & Overlays */}
      <Modal
        isOpen={isModalOpen}
        onClose={() => setIsModalOpen(false)}
        title="Scale + Fade Modal Animation"
        subtitle="Standardized Framer Motion Easing"
      >
        <div className="space-y-3 text-xs text-slate-300">
          <p>
            This modal uses the <code className="text-purple-300 font-mono">fadeInScale</code> variant with custom cubic-bezier <code className="text-purple-300 font-mono">[0.16, 1, 0.3, 1]</code> for snappy, responsive feel.
          </p>
          <div className="flex justify-end pt-3">
            <Button variant="primary" size="sm" onClick={() => setIsModalOpen(false)}>
              Got it
            </Button>
          </div>
        </div>
      </Modal>

      <Drawer
        isOpen={isDrawerOpen}
        onClose={() => setIsDrawerOpen(false)}
        title="Slide-in Drawer Animation"
        subtitle="Spring Damped Transition"
      >
        <div className="space-y-3 text-xs text-slate-300">
          <p>
            Standardized sliding panel configured with stiffness 260 and damping 28 to eliminate jitter.
          </p>
        </div>
      </Drawer>

      {/* Toast Notification */}
      <AnimatePresence>
        {showToast && (
          <motion.div
            initial={{ opacity: 0, y: 20, scale: 0.9 }}
            animate={{ opacity: 1, y: 0, scale: 1 }}
            exit={{ opacity: 0, y: 20, scale: 0.9 }}
            className="fixed top-20 right-8 z-[130] p-4 rounded-xl bg-purple-950/90 border border-purple-500/50 shadow-2xl flex items-center gap-3"
          >
            <Sparkles className="w-5 h-5 text-purple-400 animate-spin" />
            <div>
              <h4 className="text-xs font-bold text-white">Daily Report Generated</h4>
              <p className="text-[11px] text-purple-200">Ruflo synchronized with GitHub & Calendar</p>
            </div>
          </motion.div>
        )}
      </AnimatePresence>
    </motion.div>
  );
};
