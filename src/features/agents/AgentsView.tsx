import React, { useState } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import { motion } from 'framer-motion';
import {
  Bot,
  Shield,
  Wrench,
  Brain,
  Activity,
  CheckCircle2,
  Clock,
  Play,
  Cpu,
  Terminal,
  ExternalLink,
  ChevronRight,
} from 'lucide-react';
import { useVaniStore } from '@/stores/useVaniStore';
import { Agent } from '@/types';
import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { Drawer } from '@/components/ui/Drawer';
import { slideUpFade } from '@/design-system/motion';

export const AgentsView: React.FC = () => {
  const { id: paramAgentId } = useParams<{ id: string }>();
  const navigate = useNavigate();
  const agents = useVaniStore((s) => s.agents);
  const tasks = useVaniStore((s) => s.tasks);

  const [selectedAgent, setSelectedAgent] = useState<Agent | null>(
    paramAgentId ? agents.find((a) => a.id === paramAgentId) || null : null
  );

  return (
    <motion.div variants={slideUpFade} initial="initial" animate="animate" className="space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl md:text-2xl font-bold text-white tracking-tight">AI Agent Mesh</h1>
          <p className="text-xs md:text-sm text-slate-400">
            Autonomous specialized agents with sandboxed tool access and capability scopes.
          </p>
        </div>
        <Badge variant="primary" dot size="sm">
          {agents.filter((a) => a.status === 'running').length} Active Agents
        </Badge>
      </div>

      {/* Agents Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
        {agents.map((agent) => (
          <Card
            key={agent.id}
            variant="interactive"
            onClick={() => {
              setSelectedAgent(agent);
              navigate(`/agents/${agent.id}`);
            }}
            className="space-y-4 flex flex-col justify-between"
          >
            <div className="space-y-3">
              <div className="flex items-start justify-between">
                <div className="flex items-center gap-3">
                  <div className="w-10 h-10 rounded-xl bg-purple-950/40 border border-purple-500/30 flex items-center justify-center text-xl shadow-inner">
                    {agent.avatar}
                  </div>
                  <div>
                    <h3 className="text-sm font-bold text-white">{agent.name}</h3>
                    <span className="text-xs text-purple-400 font-mono">{agent.role}</span>
                  </div>
                </div>
                <Badge
                  variant={agent.status === 'running' ? 'primary' : 'neutral'}
                  dot={agent.status === 'running'}
                  size="xs"
                >
                  {agent.status}
                </Badge>
              </div>

              <p className="text-xs text-slate-300 leading-relaxed">{agent.description}</p>

              {/* Current task preview */}
              {agent.currentTaskTitle && (
                <div className="p-2.5 rounded-xl bg-white/[0.03] border border-white/5 space-y-1.5 text-xs">
                  <div className="flex items-center justify-between text-[11px] font-mono text-slate-400">
                    <span>Current Task</span>
                    <span className="text-purple-300">{agent.progress}%</span>
                  </div>
                  <p className="text-white font-medium truncate">{agent.currentTaskTitle}</p>
                </div>
              )}
            </div>

            {/* Footer metrics & tools */}
            <div className="space-y-2 pt-3 border-t border-white/5 text-xs">
              <div className="flex items-center justify-between text-slate-400 font-mono text-[11px]">
                <span>Model: {agent.model.split(' ')[0]}</span>
                <span>{agent.completedTasksCount} tasks done</span>
              </div>
              <div className="flex items-center gap-1.5 flex-wrap">
                {agent.tools.map((t, idx) => (
                  <span
                    key={idx}
                    className="text-[10px] font-mono px-2 py-0.5 rounded bg-white/[0.04] text-slate-300 border border-white/5"
                  >
                    {t}
                  </span>
                ))}
              </div>
            </div>
          </Card>
        ))}
      </div>

      {/* Agent Detail Drawer */}
      <Drawer
        isOpen={!!selectedAgent}
        onClose={() => {
          setSelectedAgent(null);
          navigate('/agents');
        }}
        title={selectedAgent ? `${selectedAgent.name} (${selectedAgent.role})` : 'Agent Details'}
        subtitle={`Agent ID: ${selectedAgent?.id}`}
        width="xl"
      >
        {selectedAgent && (
          <div className="space-y-6">
            {/* Header info */}
            <div className="p-4 rounded-xl bg-white/[0.03] border border-white/10 flex items-center justify-between">
              <div className="flex items-center gap-3">
                <span className="text-3xl">{selectedAgent.avatar}</span>
                <div>
                  <h4 className="text-sm font-bold text-white">{selectedAgent.name}</h4>
                  <p className="text-xs text-purple-300 font-mono">{selectedAgent.model}</p>
                </div>
              </div>
              <Badge variant="success" dot size="sm">
                {selectedAgent.health}
              </Badge>
            </div>

            {/* Description */}
            <div className="space-y-1.5">
              <h4 className="text-xs uppercase font-mono text-slate-400 font-semibold">Specialization</h4>
              <p className="text-xs text-slate-200 leading-relaxed bg-white/[0.02] p-3 rounded-lg border border-white/5">
                {selectedAgent.description}
              </p>
            </div>

            {/* Capability Matrix */}
            <div className="space-y-2">
              <h4 className="text-xs uppercase font-mono text-slate-400 font-semibold">
                Autonomous Capabilities
              </h4>
              <div className="space-y-1.5">
                {selectedAgent.capabilities.map((cap) => (
                  <div
                    key={cap.id}
                    className="p-2.5 rounded-lg bg-black/30 border border-white/5 flex items-center justify-between text-xs"
                  >
                    <span className="text-slate-200 font-medium">{cap.name}</span>
                    <Badge variant="cyan" size="xs">Enabled</Badge>
                  </div>
                ))}
              </div>
            </div>

            {/* Sandboxed Permissions */}
            <div className="space-y-2">
              <h4 className="text-xs uppercase font-mono text-slate-400 font-semibold flex items-center gap-1.5">
                <Shield className="w-3.5 h-3.5 text-amber-400" />
                Sandboxed Permissions
              </h4>
              <div className="flex flex-wrap gap-1.5">
                {selectedAgent.permissions.map((perm, i) => (
                  <span
                    key={i}
                    className="text-xs font-mono px-2.5 py-1 rounded-lg bg-amber-950/20 border border-amber-500/30 text-amber-300"
                  >
                    {perm}
                  </span>
                ))}
              </div>
            </div>

            {/* Memory Access Level */}
            <div className="p-3.5 rounded-xl bg-purple-950/20 border border-purple-500/30 flex items-center justify-between text-xs">
              <div className="flex items-center gap-2 text-purple-300 font-medium">
                <Brain className="w-4 h-4" />
                <span>Memory Access Policy</span>
              </div>
              <Badge variant="primary" size="xs">
                {selectedAgent.memoryAccess.toUpperCase()}
              </Badge>
            </div>

            {/* Performance Stats */}
            <div className="grid grid-cols-2 gap-3 text-xs font-mono">
              <div className="p-3 rounded-xl bg-white/[0.02] border border-white/5">
                <span className="text-slate-400 block text-[10px]">Avg Latency</span>
                <span className="text-white font-bold text-sm">{selectedAgent.avgLatencyMs} ms</span>
              </div>
              <div className="p-3 rounded-xl bg-white/[0.02] border border-white/5">
                <span className="text-slate-400 block text-[10px]">Completed Runs</span>
                <span className="text-white font-bold text-sm">{selectedAgent.completedTasksCount}</span>
              </div>
            </div>
          </div>
        )}
      </Drawer>
    </motion.div>
  );
};
