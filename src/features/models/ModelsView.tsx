import React from 'react';
import { motion } from 'framer-motion';
import {
  Cpu,
  Cloud,
  HardDrive,
  Zap,
  CheckCircle2,
  AlertCircle,
  Eye,
  Wrench,
  Code2,
  Volume2,
  Clock,
  Shield,
  WifiOff,
} from 'lucide-react';
import { useVaniStore } from '@/stores/useVaniStore';
import { Model } from '@/types';
import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { slideUpFade } from '@/design-system/motion';

export const ModelsView: React.FC = () => {
  const models = useVaniStore((s) => s.models);
  const setActiveModel = useVaniStore((s) => s.setActiveModel);
  const isOffline = useVaniStore((s) => s.isOffline);
  const toggleOffline = useVaniStore((s) => s.toggleOffline);

  return (
    <motion.div variants={slideUpFade} initial="initial" animate="animate" className="space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl md:text-2xl font-bold text-white tracking-tight">AI Models & Runtime Engines</h1>
          <p className="text-xs md:text-sm text-slate-400">
            Local-first hardware acceleration paired with optional cloud reasoning mesh.
          </p>
        </div>

        {/* Offline Mode Banner Switch */}
        <button
          onClick={toggleOffline}
          className={`flex items-center gap-2 px-3 py-1.5 rounded-xl border text-xs font-semibold transition-all ${
            isOffline
              ? 'bg-amber-950/30 border-amber-500/40 text-amber-300 shadow-[0_0_15px_rgba(245,158,11,0.2)]'
              : 'bg-white/[0.04] border-white/10 text-slate-300 hover:text-white'
          }`}
        >
          <WifiOff className="w-4 h-4" />
          <span>{isOffline ? 'Offline Active (Local Only)' : 'Cloud Mesh Active'}</span>
        </button>
      </div>

      {/* Local-First Architecture Explainer */}
      {isOffline && (
        <div className="p-4 rounded-xl bg-amber-950/20 border border-amber-500/30 text-amber-200 text-xs flex items-start gap-3">
          <Shield className="w-5 h-5 flex-shrink-0 text-amber-400 mt-0.5" />
          <div>
            <h4 className="font-semibold text-amber-300">Local-Only Operating Mode Enforced</h4>
            <p className="mt-1 leading-relaxed text-amber-200/90">
              All cloud providers (Gemini, Claude) have been safely paused. VANI is executing all voice recognition, agent synthesis, and deterministic system tools entirely on local NPU/GPU through Ollama & llama.cpp.
            </p>
          </div>
        </div>
      )}

      {/* Models Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-5">
        {models.map((model) => {
          const isPausedOffline = isOffline && model.type === 'cloud';

          return (
            <Card
              key={model.id}
              className={`space-y-4 ${
                model.status === 'active'
                  ? 'border-purple-500/50 shadow-lg shadow-purple-950/20'
                  : ''
              } ${isPausedOffline ? 'opacity-60' : ''}`}
            >
              <div className="flex items-start justify-between">
                <div className="flex items-center gap-3">
                  <div
                    className={`w-10 h-10 rounded-xl border flex items-center justify-center ${
                      model.type === 'local'
                        ? 'bg-purple-950/40 border-purple-500/30 text-purple-400'
                        : 'bg-cyan-950/40 border-cyan-500/30 text-cyan-400'
                    }`}
                  >
                    {model.type === 'local' ? <HardDrive className="w-5 h-5" /> : <Cloud className="w-5 h-5" />}
                  </div>
                  <div>
                    <h3 className="text-sm font-bold text-white flex items-center gap-2">
                      {model.name}
                      {model.status === 'active' && (
                        <Badge variant="primary" size="xs">Primary</Badge>
                      )}
                    </h3>
                    <span className="text-xs text-slate-400 font-mono">{model.provider}</span>
                  </div>
                </div>

                <Badge
                  variant={
                    isPausedOffline
                      ? 'warning'
                      : model.status === 'active'
                      ? 'success'
                      : 'neutral'
                  }
                  size="xs"
                  dot={model.status === 'active'}
                >
                  {isPausedOffline ? 'Paused (Offline)' : model.status}
                </Badge>
              </div>

              <p className="text-xs text-slate-300 leading-relaxed">{model.description}</p>

              {/* Context Window Capacity Bar */}
              <div className="space-y-1.5 pt-1">
                <div className="flex items-center justify-between text-[11px] font-mono text-slate-400">
                  <span>Context Window</span>
                  <span className="text-purple-300">{model.contextSize}</span>
                </div>
                <div className="w-full h-1.5 bg-white/10 rounded-full overflow-hidden">
                  <div
                    className="h-full bg-gradient-to-r from-purple-500 to-cyan-400 rounded-full"
                    style={{
                      width: `${Math.min(
                        100,
                        (model.activeContextTokens / model.maxTokens) * 100
                      )}%`,
                    }}
                  />
                </div>
              </div>

              {/* Latency & Capabilities Chips */}
              <div className="grid grid-cols-2 gap-2 text-xs font-mono">
                <div className="p-2 rounded-lg bg-black/30 border border-white/5">
                  <span className="text-slate-400 text-[10px] block">Inference TTFT</span>
                  <span className="text-white font-bold">{model.ttftMs} ms</span>
                </div>
                <div className="p-2 rounded-lg bg-black/30 border border-white/5">
                  <span className="text-slate-400 text-[10px] block">Engine Type</span>
                  <span className="text-purple-300 font-bold capitalize">{model.type}</span>
                </div>
              </div>

              {/* Footer Switch Action */}
              <div className="pt-3 border-t border-white/5 flex items-center justify-between">
                <div className="flex items-center gap-1.5">
                  {model.capabilities.map((c, i) => (
                    <span key={i} className="text-[10px] font-mono px-1.5 py-0.2 rounded bg-white/[0.04] text-slate-400">
                      {c}
                    </span>
                  ))}
                </div>
                <Button
                  variant={model.status === 'active' ? 'secondary' : 'primary'}
                  size="xs"
                  disabled={model.status === 'active' || isPausedOffline}
                  onClick={() => setActiveModel(model.id)}
                >
                  {model.status === 'active' ? 'Active Default' : 'Set as Primary'}
                </Button>
              </div>
            </Card>
          );
        })}
      </div>
    </motion.div>
  );
};
