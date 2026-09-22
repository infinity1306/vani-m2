import React, { useState } from 'react';
import { motion } from 'framer-motion';
import {
  Terminal,
  Activity,
  Cpu,
  RefreshCw,
  Trash2,
  Filter,
  Zap,
  Layers,
  ArrowRight,
} from 'lucide-react';
import { useVaniStore } from '@/stores/useVaniStore';
import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { slideUpFade } from '@/design-system/motion';

export const DeveloperConsoleView: React.FC = () => {
  const activity = useVaniStore((s) => s.activity);
  const systemMetrics = useVaniStore((s) => s.systemMetrics);

  const [filterType, setFilterType] = useState<string>('all');

  const filteredLogs = activity.filter(
    (a) => filterType === 'all' || a.type === filterType
  );

  return (
    <motion.div variants={slideUpFade} initial="initial" animate="animate" className="space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl md:text-2xl font-bold text-white tracking-tight">Developer Console & IPC Stream</h1>
          <p className="text-xs md:text-sm text-slate-400">
            Real-time inter-process communication event bus, acoustic telemetry, and sub-agent state transitions.
          </p>
        </div>
        <div className="flex items-center gap-2">
          <Badge variant="cyan" dot size="sm">
            IPC Socket: ws://127.0.0.1:8765
          </Badge>
        </div>
      </div>

      {/* Latency & Diagnostics Strip */}
      <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 font-mono text-xs">
        <div className="p-3 rounded-xl bg-[#090d16] border border-white/10">
          <span className="text-slate-500 text-[10px] block uppercase">IPC Loopback Latency</span>
          <span className="text-emerald-400 font-bold text-base">0.42 ms</span>
        </div>
        <div className="p-3 rounded-xl bg-[#090d16] border border-white/10">
          <span className="text-slate-500 text-[10px] block uppercase">Local Audio Pipeline</span>
          <span className="text-cyan-400 font-bold text-base">28 ms (Whisper)</span>
        </div>
        <div className="p-3 rounded-xl bg-[#090d16] border border-white/10">
          <span className="text-slate-500 text-[10px] block uppercase">NPU Memory Bound</span>
          <span className="text-purple-400 font-bold text-base">4.8 GB / 16 GB</span>
        </div>
        <div className="p-3 rounded-xl bg-[#090d16] border border-white/10">
          <span className="text-slate-500 text-[10px] block uppercase">Total Event Traces</span>
          <span className="text-white font-bold text-base">{activity.length}</span>
        </div>
      </div>

      {/* Filter Tabs */}
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-1.5 p-1 rounded-xl bg-white/[0.03] border border-white/10 overflow-x-auto">
          {(['all', 'voice', 'agent', 'task', 'tool', 'security'] as const).map((type) => (
            <button
              key={type}
              onClick={() => setFilterType(type)}
              className={`px-3 py-1 rounded-lg text-xs font-medium capitalize transition-all whitespace-nowrap ${
                filterType === type ? 'bg-purple-600 text-white' : 'text-slate-400 hover:text-white'
              }`}
            >
              {type}
            </button>
          ))}
        </div>
      </div>

      {/* Terminal Log Stream */}
      <Card className="p-0 overflow-hidden bg-black/90 border-purple-500/20 shadow-2xl">
        <div className="flex items-center justify-between px-4 py-2.5 bg-white/[0.04] border-b border-white/10 text-xs font-mono text-slate-400">
          <div className="flex items-center gap-2">
            <Terminal className="w-3.5 h-3.5 text-purple-400" />
            <span className="text-slate-300">vani-runtime-ipc.log</span>
          </div>
          <span className="text-[10px] text-emerald-400">● LIVE POLLING</span>
        </div>

        <div className="p-4 space-y-2 font-mono text-xs max-h-[500px] overflow-y-auto">
          {filteredLogs.map((log) => (
            <div key={log.id} className="flex items-start gap-3 text-slate-300 leading-relaxed hover:bg-white/[0.02] p-1 rounded">
              <span className="text-slate-500 text-[11px] flex-shrink-0">[{log.timestamp}]</span>
              <Badge variant="outline" size="xs" className="uppercase text-[9px] flex-shrink-0">
                {log.type}
              </Badge>
              <div className="flex-1 min-w-0">
                {log.agent && <strong className="text-purple-300 font-semibold">{log.agent}: </strong>}
                <span className="text-slate-200">{log.action}</span>
                {log.target && <span className="text-cyan-400 ml-2">({log.target})</span>}
              </div>
            </div>
          ))}
        </div>
      </Card>
    </motion.div>
  );
};
