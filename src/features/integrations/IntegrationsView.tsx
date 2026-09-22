import React, { useState } from 'react';
import { motion } from 'framer-motion';
import {
  Layers,
  Github,
  Mail,
  Calendar,
  MessageSquare,
  BookOpen,
  GraduationCap,
  Users,
  CheckCircle2,
  AlertTriangle,
  RefreshCw,
  ExternalLink,
  Shield,
  Plus,
} from 'lucide-react';
import { useVaniStore } from '@/stores/useVaniStore';
import { Integration } from '@/types';
import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { Drawer } from '@/components/ui/Drawer';
import { slideUpFade } from '@/design-system/motion';

export const IntegrationsView: React.FC = () => {
  const integrations = useVaniStore((s) => s.integrations);
  const toggleIntegrationConnection = useVaniStore((s) => s.toggleIntegrationConnection);

  const [activeCategory, setActiveCategory] = useState<string>('all');
  const [selectedIntegration, setSelectedIntegration] = useState<Integration | null>(null);

  const categories = ['all', 'Development', 'Communication', 'Productivity', 'Education', 'Business'];

  const getIcon = (iconName: string) => {
    switch (iconName) {
      case 'Github':
        return <Github className="w-5 h-5 text-purple-400" />;
      case 'Mail':
        return <Mail className="w-5 h-5 text-cyan-400" />;
      case 'Calendar':
        return <Calendar className="w-5 h-5 text-emerald-400" />;
      case 'GraduationCap':
        return <GraduationCap className="w-5 h-5 text-amber-400" />;
      case 'Users':
        return <Users className="w-5 h-5 text-blue-400" />;
      case 'MessageSquare':
        return <MessageSquare className="w-5 h-5 text-indigo-400" />;
      default:
        return <BookOpen className="w-5 h-5 text-pink-400" />;
    }
  };

  const filtered = integrations.filter(
    (i) => activeCategory === 'all' || i.category === activeCategory
  );

  return (
    <motion.div variants={slideUpFade} initial="initial" animate="animate" className="space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl md:text-2xl font-bold text-white tracking-tight">Integration Hub & Connectors</h1>
          <p className="text-xs md:text-sm text-slate-400">
            Connect VANI to your developer ecosystem, communications, CRM pipelines, and LMS courseware.
          </p>
        </div>
        <div className="flex items-center gap-1.5 p-1 rounded-xl bg-white/[0.03] border border-white/10 overflow-x-auto">
          {categories.map((c) => (
            <button
              key={c}
              onClick={() => setActiveCategory(c)}
              className={`px-3 py-1 rounded-lg text-xs font-medium capitalize transition-all whitespace-nowrap ${
                activeCategory === c ? 'bg-purple-600 text-white' : 'text-slate-400 hover:text-white'
              }`}
            >
              {c}
            </button>
          ))}
        </div>
      </div>

      {/* Domain Readiness Modules (CRM & LMS Highlights) */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        {/* CRM Module Card */}
        <Card variant="glow" className="space-y-3">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-2.5">
              <div className="p-2 rounded-xl bg-blue-500/20 text-blue-400">
                <Users className="w-5 h-5" />
              </div>
              <div>
                <h3 className="text-sm font-bold text-white">Enterprise CRM Intelligence</h3>
                <span className="text-xs text-slate-400 font-mono">Leads • Pipeline • Activities</span>
              </div>
            </div>
            <Badge variant="success" size="xs" dot>Sync Active</Badge>
          </div>
          <p className="text-xs text-slate-300">
            VANI automatically enriches lead data, drafts follow-up communications, and updates deal stages upon voice command.
          </p>
          <div className="grid grid-cols-3 gap-2 pt-2 border-t border-white/5 text-center font-mono text-xs">
            <div className="p-2 rounded-lg bg-black/40">
              <span className="text-slate-400 text-[10px] block">Open Leads</span>
              <span className="text-white font-bold">14</span>
            </div>
            <div className="p-2 rounded-lg bg-black/40">
              <span className="text-slate-400 text-[10px] block">Pipeline Value</span>
              <span className="text-purple-300 font-bold">$180k</span>
            </div>
            <div className="p-2 rounded-lg bg-black/40">
              <span className="text-slate-400 text-[10px] block">Follow-ups</span>
              <span className="text-cyan-300 font-bold">6 Today</span>
            </div>
          </div>
        </Card>

        {/* LMS Module Card */}
        <Card variant="glow" className="space-y-3">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-2.5">
              <div className="p-2 rounded-xl bg-amber-500/20 text-amber-400">
                <GraduationCap className="w-5 h-5" />
              </div>
              <div>
                <h3 className="text-sm font-bold text-white">Canvas LMS Academic Assistant</h3>
                <span className="text-xs text-slate-400 font-mono">Courses • Assignments • Gradebook</span>
              </div>
            </div>
            <Badge variant="success" size="xs" dot>Sync Active</Badge>
          </div>
          <p className="text-xs text-slate-300">
            Indexes course syllabi, flags upcoming student submissions, and auto-generates rubric evaluation drafts.
          </p>
          <div className="grid grid-cols-3 gap-2 pt-2 border-t border-white/5 text-center font-mono text-xs">
            <div className="p-2 rounded-lg bg-black/40">
              <span className="text-slate-400 text-[10px] block">Enrolled Courses</span>
              <span className="text-white font-bold">4</span>
            </div>
            <div className="p-2 rounded-lg bg-black/40">
              <span className="text-slate-400 text-[10px] block">Submissions Due</span>
              <span className="text-amber-300 font-bold">2</span>
            </div>
            <div className="p-2 rounded-lg bg-black/40">
              <span className="text-slate-400 text-[10px] block">Class Avg</span>
              <span className="text-emerald-300 font-bold">91.4%</span>
            </div>
          </div>
        </Card>
      </div>

      {/* Integrations Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-5">
        {filtered.map((item) => (
          <Card key={item.id} className="flex flex-col justify-between space-y-4">
            <div className="space-y-3">
              <div className="flex items-start justify-between">
                <div className="flex items-center gap-3">
                  <div className="w-10 h-10 rounded-xl bg-white/[0.04] border border-white/10 flex items-center justify-center">
                    {getIcon(item.icon)}
                  </div>
                  <div>
                    <h3 className="text-sm font-bold text-white">{item.name}</h3>
                    <span className="text-xs text-slate-400 font-mono">{item.category}</span>
                  </div>
                </div>
                <Badge
                  variant={
                    item.status === 'connected'
                      ? 'success'
                      : item.status === 'needs_attention'
                      ? 'warning'
                      : 'neutral'
                  }
                  size="xs"
                  dot={item.status === 'connected'}
                >
                  {item.status.replace('_', ' ')}
                </Badge>
              </div>

              <p className="text-xs text-slate-300 leading-relaxed">{item.description}</p>

              {item.account && (
                <div className="p-2 rounded-lg bg-white/[0.02] border border-white/5 text-[11px] font-mono text-purple-300">
                  Account: {item.account}
                </div>
              )}
            </div>

            <div className="pt-3 border-t border-white/5 flex items-center justify-between">
              <button
                onClick={() => setSelectedIntegration(item)}
                className="text-xs text-slate-400 hover:text-white flex items-center gap-1 transition-colors"
              >
                <span>Config</span>
                <ExternalLink className="w-3 h-3" />
              </button>
              <Button
                variant={item.status === 'connected' ? 'secondary' : 'primary'}
                size="xs"
                onClick={() => toggleIntegrationConnection(item.id)}
              >
                {item.status === 'connected' ? 'Disconnect' : 'Connect'}
              </Button>
            </div>
          </Card>
        ))}
      </div>

      {/* Integration Detail Drawer */}
      <Drawer
        isOpen={!!selectedIntegration}
        onClose={() => setSelectedIntegration(null)}
        title={selectedIntegration ? selectedIntegration.name : 'Integration Config'}
        subtitle={`Connector ID: ${selectedIntegration?.id}`}
      >
        {selectedIntegration && (
          <div className="space-y-5 text-xs">
            <div className="p-3.5 rounded-xl bg-white/[0.03] border border-white/10 flex items-center justify-between">
              <span className="text-white font-medium">Status</span>
              <Badge variant={selectedIntegration.status === 'connected' ? 'success' : 'neutral'} dot>
                {selectedIntegration.status}
              </Badge>
            </div>

            <div className="space-y-1.5">
              <h4 className="font-mono uppercase text-slate-400 font-semibold">Granted Permissions</h4>
              <div className="flex flex-wrap gap-1.5">
                {selectedIntegration.permissions.map((p, idx) => (
                  <span key={idx} className="px-2.5 py-1 rounded-lg bg-purple-950/30 border border-purple-500/30 text-purple-300 font-mono">
                    {p}
                  </span>
                ))}
              </div>
            </div>

            <div className="space-y-1.5">
              <h4 className="font-mono uppercase text-slate-400 font-semibold">Tools Provided to Agents</h4>
              <div className="space-y-1">
                {selectedIntegration.toolsProvided.map((tool, idx) => (
                  <div key={idx} className="p-2 rounded-lg bg-black/30 border border-white/5 text-slate-200">
                    ✓ {tool}
                  </div>
                ))}
              </div>
            </div>

            <div className="pt-4 border-t border-white/10 flex justify-between">
              <Button
                variant="danger"
                size="xs"
                onClick={() => {
                  toggleIntegrationConnection(selectedIntegration.id);
                  setSelectedIntegration(null);
                }}
              >
                Revoke All Permissions
              </Button>
            </div>
          </div>
        )}
      </Drawer>
    </motion.div>
  );
};
