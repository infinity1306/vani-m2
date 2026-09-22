import React from 'react';
import { useNavigate } from 'react-router-dom';
import { motion } from 'framer-motion';
import {
  Sparkles,
  Terminal,
  Camera,
  FolderCode,
  FilePlus,
  Search,
  Globe,
  Cpu,
  HardDrive,
  Activity,
  Wifi,
  Eye,
  Mic,
  Video,
  ScanText,
  Layers,
  ArrowRight,
  Play,
  CheckCircle2,
  AlertTriangle,
  Shield,
  Bot,
  Brain,
  Zap,
} from 'lucide-react';
import { ConcentricVoiceOrb } from '@/components/voice/ConcentricVoiceOrb';
import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { useVaniStore } from '@/stores/useVaniStore';
import { useVoiceStore } from '@/stores/useVoiceStore';
import { slideUpFade } from '@/design-system/motion';

export const DashboardView: React.FC = () => {
  const navigate = useNavigate();
  const tasks = useVaniStore((s) => s.tasks);
  const agents = useVaniStore((s) => s.agents);
  const models = useVaniStore((s) => s.models);
  const systemMetrics = useVaniStore((s) => s.systemMetrics);
  const activity = useVaniStore((s) => s.activity);
  const integrations = useVaniStore((s) => s.integrations);
  const isOffline = useVaniStore((s) => s.isOffline);
  const setSelectedTaskId = useVaniStore((s) => s.setSelectedTaskId);
  const triggerVoicePrompt = useVoiceStore((s) => s.triggerVoicePrompt);

  const activeTasks = tasks.filter((t) => t.state === 'running');
  const activeModel = models.find((m) => m.status === 'active') || models[0];

  const quickPrompts = [
    { label: 'Open VS Code', action: 'Open VS Code and navigate to React auth project' },
    { label: 'Summarize today\'s news', action: 'Summarize today\'s top AI research news' },
    { label: 'Research New AI Tools', action: 'Research 2026 AI agent frameworks' },
    { label: 'Show my tasks', action: 'Show all running tasks and progress' },
  ];

  return (
    <motion.div
      variants={slideUpFade}
      initial="initial"
      animate="animate"
      className="space-y-6"
    >
      {/* 1. Hero Voice Interaction Area */}
      <Card variant="elevated" className="relative overflow-hidden border-purple-500/20 py-8 px-6 text-center">
        {/* Subtle grid background */}
        <div className="absolute inset-0 bg-[radial-gradient(#8b5cf6_1px,transparent_1px)] [background-size:24px_24px] opacity-10" />

        <div className="relative z-10 max-w-2xl mx-auto flex flex-col items-center">
          <div className="flex items-center gap-2 mb-3">
            <Badge variant="primary" dot size="sm">
              {isOffline ? 'Local Engine (Ollama)' : 'VANI Core 2.4 Active'}
            </Badge>
            <span className="text-xs font-mono text-slate-400">TTFT: 320ms</span>
          </div>

          <h1 className="text-xl md:text-3xl font-bold tracking-tight text-white mb-1">
            Good Morning, <span className="text-gradient-purple">Anant</span>
          </h1>
          <p className="text-sm md:text-base text-slate-300 mb-6 font-medium">
            How can I help you today?
          </p>

          {/* Central Concentric Voice Orb */}
          <div className="my-2">
            <ConcentricVoiceOrb size="hero" showControls />
          </div>

          {/* Voice Prompt Suggestions */}
          <div className="mt-6 flex flex-wrap items-center justify-center gap-2">
            <span className="text-xs text-slate-400 font-medium mr-1">Try saying:</span>
            {quickPrompts.map((qp, idx) => (
              <button
                key={idx}
                onClick={() => triggerVoicePrompt(qp.action)}
                className="text-xs px-3 py-1.5 rounded-full bg-white/[0.05] hover:bg-purple-600/25 border border-white/10 hover:border-purple-500/40 text-slate-300 hover:text-white transition-all shadow-sm"
              >
                {qp.label}
              </button>
            ))}
          </div>
        </div>
      </Card>

      {/* 2. Primary Operating Grid */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Left Column: Active Tasks & Running Agents */}
        <div className="space-y-6 lg:col-span-2">
          {/* Active Tasks Center */}
          <Card className="space-y-4">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-2.5">
                <div className="p-1.5 rounded-lg bg-purple-500/10 text-purple-400">
                  <Activity className="w-4 h-4" />
                </div>
                <div>
                  <h3 className="text-sm font-semibold text-white">Active Tasks</h3>
                  <p className="text-xs text-slate-400">{activeTasks.length} agents currently executing</p>
                </div>
              </div>
              <Button
                variant="ghost"
                size="xs"
                rightIcon={<ArrowRight className="w-3 h-3" />}
                onClick={() => navigate('/tasks')}
              >
                View all tasks
              </Button>
            </div>

            <div className="space-y-3">
              {activeTasks.map((task) => (
                <div
                  key={task.id}
                  onClick={() => {
                    setSelectedTaskId(task.id);
                    navigate(`/tasks/${task.id}`);
                  }}
                  className="p-3.5 rounded-xl bg-white/[0.03] hover:bg-purple-950/20 border border-white/5 hover:border-purple-500/30 transition-all cursor-pointer space-y-2 group"
                >
                  <div className="flex items-center justify-between">
                    <div className="flex items-center gap-2.5 min-w-0">
                      <span className="text-base">{task.agentAvatar || '⚡'}</span>
                      <div className="min-w-0">
                        <h4 className="text-xs font-semibold text-white truncate group-hover:text-purple-300 transition-colors">
                          {task.title}
                        </h4>
                        <span className="text-[11px] text-slate-400 font-mono">
                          {task.agentName} ({task.category})
                        </span>
                      </div>
                    </div>
                    <span className="text-xs font-mono font-bold text-purple-400">
                      {task.progress}%
                    </span>
                  </div>

                  {/* Progress Bar */}
                  <div className="w-full h-1.5 bg-white/10 rounded-full overflow-hidden">
                    <motion.div
                      initial={{ width: 0 }}
                      animate={{ width: `${task.progress}%` }}
                      transition={{ duration: 0.8, ease: 'easeOut' }}
                      className="h-full bg-gradient-to-r from-purple-500 to-cyan-400 rounded-full shadow-[0_0_8px_#a855f7]"
                    />
                  </div>

                  {/* Tools snippet */}
                  {task.toolsUsed && task.toolsUsed.length > 0 && (
                    <div className="flex items-center gap-2 text-[11px] text-slate-400 pt-1">
                      <span className="text-slate-400">Tools:</span>
                      {task.toolsUsed.map((t, idx) => (
                        <span key={idx} className="px-1.5 py-0.2 rounded bg-white/[0.05] text-slate-300 font-mono text-[10px]">
                          ✓ {t.name}
                        </span>
                      ))}
                    </div>
                  )}
                </div>
              ))}
            </div>
          </Card>

          {/* Quick Actions Toolbar */}
          <Card className="space-y-3">
            <h3 className="text-xs font-semibold text-slate-400 uppercase tracking-wider font-mono">
              Quick System Actions
            </h3>
            <div className="grid grid-cols-2 sm:grid-cols-4 gap-2.5">
              <button
                onClick={() => navigate('/tools')}
                className="flex items-center gap-2 p-2.5 rounded-xl bg-white/[0.03] hover:bg-white/[0.07] border border-white/10 text-xs font-medium text-slate-200 transition-all hover:translate-y-[-1px]"
              >
                <Camera className="w-4 h-4 text-cyan-400" />
                <span>Screenshot</span>
              </button>
              <button
                onClick={() => navigate('/tools')}
                className="flex items-center gap-2 p-2.5 rounded-xl bg-white/[0.03] hover:bg-white/[0.07] border border-white/10 text-xs font-medium text-slate-200 transition-all hover:translate-y-[-1px]"
              >
                <Terminal className="w-4 h-4 text-purple-400" />
                <span>Terminal</span>
              </button>
              <button
                onClick={() => triggerVoicePrompt('Open VS Code project')}
                className="flex items-center gap-2 p-2.5 rounded-xl bg-white/[0.03] hover:bg-white/[0.07] border border-white/10 text-xs font-medium text-slate-200 transition-all hover:translate-y-[-1px]"
              >
                <FolderCode className="w-4 h-4 text-emerald-400" />
                <span>Open Code</span>
              </button>
              <button
                onClick={() => navigate('/tasks')}
                className="flex items-center gap-2 p-2.5 rounded-xl bg-white/[0.03] hover:bg-white/[0.07] border border-white/10 text-xs font-medium text-slate-200 transition-all hover:translate-y-[-1px]"
              >
                <FilePlus className="w-4 h-4 text-amber-400" />
                <span>Create Task</span>
              </button>
            </div>
          </Card>

          {/* Live Activity Timeline */}
          <Card className="space-y-3">
            <div className="flex items-center justify-between">
              <h3 className="text-sm font-semibold text-white">Live Activity</h3>
              <Button
                variant="ghost"
                size="xs"
                rightIcon={<ArrowRight className="w-3 h-3" />}
                onClick={() => navigate('/developer')}
              >
                View full stream
              </Button>
            </div>

            <div className="space-y-2.5">
              {activity.slice(0, 4).map((evt) => (
                <div
                  key={evt.id}
                  className="flex items-center justify-between p-2.5 rounded-lg bg-white/[0.02] border border-white/5 text-xs"
                >
                  <div className="flex items-center gap-3 min-w-0">
                    <span className="text-[11px] font-mono text-slate-400">{evt.timestamp}</span>
                    <span className="w-1.5 h-1.5 rounded-full bg-purple-400 flex-shrink-0" />
                    <span className="text-slate-200 truncate">
                      {evt.agent ? <strong className="text-purple-300 font-semibold">{evt.agent}: </strong> : null}
                      {evt.action}
                    </span>
                  </div>
                  {evt.target && (
                    <span className="hidden sm:inline text-[11px] text-slate-400 font-mono truncate max-w-[180px]">
                      {evt.target}
                    </span>
                  )}
                </div>
              ))}
            </div>
          </Card>
        </div>

        {/* Right Column: System Status, Agents Health, Perception, Integrations */}
        <div className="space-y-6">
          {/* System Telemetry Status */}
          <Card className="space-y-4">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-2">
                <Cpu className="w-4 h-4 text-purple-400" />
                <h3 className="text-sm font-semibold text-white">System Status</h3>
              </div>
              <Badge variant="success" size="xs" dot>
                Optimal
              </Badge>
            </div>

            {/* Gauges */}
            <div className="space-y-3">
              <div>
                <div className="flex justify-between text-xs font-mono text-slate-300 mb-1">
                  <span>CPU Load</span>
                  <span>{systemMetrics.cpuUsage}%</span>
                </div>
                <div className="w-full h-1.5 bg-white/10 rounded-full overflow-hidden">
                  <div
                    className="h-full bg-purple-500 rounded-full"
                    style={{ width: `${systemMetrics.cpuUsage}%` }}
                  />
                </div>
              </div>

              <div>
                <div className="flex justify-between text-xs font-mono text-slate-300 mb-1">
                  <span>RAM ({systemMetrics.ramUsedGb}GB / {systemMetrics.ramTotalGb}GB)</span>
                  <span>{Math.round((systemMetrics.ramUsedGb / systemMetrics.ramTotalGb) * 100)}%</span>
                </div>
                <div className="w-full h-1.5 bg-white/10 rounded-full overflow-hidden">
                  <div
                    className="h-full bg-cyan-500 rounded-full"
                    style={{ width: `${(systemMetrics.ramUsedGb / systemMetrics.ramTotalGb) * 100}%` }}
                  />
                </div>
              </div>

              <div>
                <div className="flex justify-between text-xs font-mono text-slate-300 mb-1">
                  <span>GPU Core (NPU)</span>
                  <span>{systemMetrics.gpuUsage}%</span>
                </div>
                <div className="w-full h-1.5 bg-white/10 rounded-full overflow-hidden">
                  <div
                    className="h-full bg-emerald-500 rounded-full"
                    style={{ width: `${systemMetrics.gpuUsage}%` }}
                  />
                </div>
              </div>

              <div>
                <div className="flex justify-between text-xs font-mono text-slate-300 mb-1">
                  <span>Disk ({systemMetrics.diskUsedGb}GB / {systemMetrics.diskTotalGb}GB)</span>
                  <span>25%</span>
                </div>
                <div className="w-full h-1.5 bg-white/10 rounded-full overflow-hidden">
                  <div className="h-full bg-indigo-500 rounded-full" style={{ width: '25%' }} />
                </div>
              </div>
            </div>

            {/* Network & Model Summary */}
            <div className="pt-2 border-t border-white/10 flex items-center justify-between text-xs text-slate-400">
              <span className="flex items-center gap-1.5">
                <Wifi className="w-3.5 h-3.5 text-emerald-400" />
                {systemMetrics.networkBandwidthMbps} Mbps ({systemMetrics.networkLatencyMs}ms)
              </span>
              <span className="font-mono text-purple-400">{activeModel.name.split(' ')[0]}</span>
            </div>
          </Card>

          {/* Active Perception Matrix */}
          <Card className="space-y-3">
            <h3 className="text-xs font-semibold text-slate-400 uppercase tracking-wider font-mono">
              Perception Layer
            </h3>
            <div className="grid grid-cols-2 gap-2 text-xs">
              <div className="flex items-center justify-between p-2 rounded-lg bg-white/[0.02] border border-white/5">
                <span className="flex items-center gap-1.5 text-slate-300">
                  <Eye className="w-3.5 h-3.5 text-cyan-400" /> Screen
                </span>
                <Badge variant="success" size="xs">Active</Badge>
              </div>
              <div className="flex items-center justify-between p-2 rounded-lg bg-white/[0.02] border border-white/5">
                <span className="flex items-center gap-1.5 text-slate-300">
                  <Mic className="w-3.5 h-3.5 text-purple-400" /> Mic
                </span>
                <Badge variant="success" size="xs">Active</Badge>
              </div>
              <div className="flex items-center justify-between p-2 rounded-lg bg-white/[0.02] border border-white/5">
                <span className="flex items-center gap-1.5 text-slate-300">
                  <Video className="w-3.5 h-3.5 text-slate-400" /> Camera
                </span>
                <Badge variant="neutral" size="xs">Off</Badge>
              </div>
              <div className="flex items-center justify-between p-2 rounded-lg bg-white/[0.02] border border-white/5">
                <span className="flex items-center gap-1.5 text-slate-300">
                  <ScanText className="w-3.5 h-3.5 text-emerald-400" /> OCR
                </span>
                <Badge variant="cyan" size="xs">Ready</Badge>
              </div>
            </div>
          </Card>

          {/* Agents Health Overview */}
          <Card className="space-y-3">
            <div className="flex items-center justify-between">
              <h3 className="text-xs font-semibold text-slate-400 uppercase tracking-wider font-mono">
                Agent Mesh ({agents.length})
              </h3>
              <Button variant="ghost" size="xs" onClick={() => navigate('/agents')}>
                Inspect
              </Button>
            </div>
            <div className="space-y-2">
              {agents.slice(0, 3).map((agent) => (
                <div
                  key={agent.id}
                  onClick={() => navigate(`/agents/${agent.id}`)}
                  className="flex items-center justify-between p-2 rounded-lg bg-white/[0.02] hover:bg-white/[0.06] border border-white/5 cursor-pointer transition-colors"
                >
                  <div className="flex items-center gap-2">
                    <span>{agent.avatar}</span>
                    <span className="text-xs font-medium text-white">{agent.name}</span>
                    <span className="text-[10px] text-slate-400">({agent.role})</span>
                  </div>
                  <Badge
                    variant={agent.status === 'running' ? 'primary' : 'neutral'}
                    size="xs"
                    dot={agent.status === 'running'}
                  >
                    {agent.status}
                  </Badge>
                </div>
              ))}
            </div>
          </Card>

          {/* Integrations Fast Status */}
          <Card className="space-y-3">
            <div className="flex items-center justify-between">
              <h3 className="text-xs font-semibold text-slate-400 uppercase tracking-wider font-mono">
                Integrations
              </h3>
              <Button variant="ghost" size="xs" onClick={() => navigate('/integrations')}>
                Manage
              </Button>
            </div>
            <div className="flex items-center gap-2 flex-wrap">
              {integrations.slice(0, 5).map((intg) => (
                <div
                  key={intg.id}
                  className={`flex items-center gap-1.5 px-2.5 py-1 rounded-lg text-xs font-medium border ${
                    intg.status === 'connected'
                      ? 'bg-emerald-950/20 border-emerald-500/30 text-emerald-300'
                      : 'bg-white/[0.03] border-white/10 text-slate-400'
                  }`}
                >
                  <span className="w-1.5 h-1.5 rounded-full bg-current" />
                  <span>{intg.name}</span>
                </div>
              ))}
            </div>
          </Card>
        </div>
      </div>
    </motion.div>
  );
};
