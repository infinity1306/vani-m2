import React, { useState } from 'react';
import { motion } from 'framer-motion';
import {
  Wrench,
  Terminal,
  Folder,
  Globe,
  Camera,
  ScanText,
  Layout,
  GitBranch,
  Volume2,
  Shield,
  CheckCircle2,
  AlertTriangle,
  Play,
  Filter,
} from 'lucide-react';
import { useVaniStore } from '@/stores/useVaniStore';
import { Tool, RiskLevel } from '@/types';
import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { slideUpFade } from '@/design-system/motion';

export const ToolsView: React.FC = () => {
  const tools = useVaniStore((s) => s.tools);
  const toggleToolStatus = useVaniStore((s) => s.toggleToolStatus);

  const [selectedRisk, setSelectedRisk] = useState<RiskLevel | 'all'>('all');

  const getToolIcon = (iconName: string) => {
    switch (iconName) {
      case 'Terminal':
        return <Terminal className="w-5 h-5 text-purple-400" />;
      case 'Folder':
        return <Folder className="w-5 h-5 text-cyan-400" />;
      case 'Globe':
        return <Globe className="w-5 h-5 text-blue-400" />;
      case 'Camera':
        return <Camera className="w-5 h-5 text-emerald-400" />;
      case 'ScanText':
        return <ScanText className="w-5 h-5 text-teal-400" />;
      case 'Layout':
        return <Layout className="w-5 h-5 text-indigo-400" />;
      case 'GitBranch':
        return <GitBranch className="w-5 h-5 text-purple-400" />;
      default:
        return <Volume2 className="w-5 h-5 text-amber-400" />;
    }
  };

  const getRiskBadge = (risk: RiskLevel) => {
    switch (risk) {
      case 'low':
        return <Badge variant="success" size="xs">Low Risk</Badge>;
      case 'medium':
        return <Badge variant="warning" size="xs">Medium Risk</Badge>;
      case 'high':
        return <Badge variant="danger" size="xs">High Risk</Badge>;
      case 'restricted':
        return <Badge variant="neutral" size="xs">Restricted</Badge>;
    }
  };

  const filteredTools = tools.filter(
    (t) => selectedRisk === 'all' || t.riskLevel === selectedRisk
  );

  return (
    <motion.div variants={slideUpFade} initial="initial" animate="animate" className="space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl md:text-2xl font-bold text-white tracking-tight">Deterministic Tool Registry</h1>
          <p className="text-xs md:text-sm text-slate-400">
            System primitives and external capabilities callable by VANI's local-first agent mesh.
          </p>
        </div>
        <div className="flex items-center gap-1.5 p-1 rounded-xl bg-white/[0.03] border border-white/10">
          {(['all', 'low', 'medium', 'high'] as const).map((r) => (
            <button
              key={r}
              onClick={() => setSelectedRisk(r)}
              className={`px-3 py-1 rounded-lg text-xs font-medium capitalize transition-all ${
                selectedRisk === r
                  ? 'bg-purple-600 text-white'
                  : 'text-slate-400 hover:text-white'
              }`}
            >
              {r}
            </button>
          ))}
        </div>
      </div>

      {/* Tools Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-5">
        {filteredTools.map((tool) => (
          <Card key={tool.id} className="flex flex-col justify-between space-y-4">
            <div className="space-y-3">
              <div className="flex items-start justify-between">
                <div className="flex items-center gap-3">
                  <div className="w-10 h-10 rounded-xl bg-white/[0.04] border border-white/10 flex items-center justify-center shadow-inner">
                    {getToolIcon(tool.icon)}
                  </div>
                  <div>
                    <h3 className="text-sm font-bold text-white">{tool.name}</h3>
                    <span className="text-xs text-slate-400 font-mono">v{tool.version}</span>
                  </div>
                </div>
                {getRiskBadge(tool.riskLevel)}
              </div>

              <p className="text-xs text-slate-300 leading-relaxed">{tool.description}</p>

              {/* Scoped Permissions */}
              <div className="space-y-1">
                <span className="text-[10px] uppercase font-mono text-slate-400">Bound Privileges:</span>
                <div className="flex flex-wrap gap-1">
                  {tool.permissions.map((p, i) => (
                    <span
                      key={i}
                      className="text-[10px] font-mono px-2 py-0.5 rounded bg-black/40 text-purple-300 border border-white/5"
                    >
                      {p}
                    </span>
                  ))}
                </div>
              </div>
            </div>

            {/* Footer controls & telemetry */}
            <div className="pt-3 border-t border-white/5 flex items-center justify-between text-xs">
              <div className="font-mono text-[11px] text-slate-400">
                <span>Executions: </span>
                <strong className="text-white">{tool.executionCount}</strong>
              </div>
              <Button
                variant={tool.status === 'active' ? 'secondary' : 'outline'}
                size="xs"
                onClick={() => toggleToolStatus(tool.id)}
              >
                {tool.status === 'active' ? 'Enabled' : 'Disabled'}
              </Button>
            </div>
          </Card>
        ))}
      </div>
    </motion.div>
  );
};
