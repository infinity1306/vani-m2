import React, { useState } from 'react';
import { motion } from 'framer-motion';
import {
  Puzzle,
  Code2,
  Download,
  CheckCircle2,
  ExternalLink,
  BookOpen,
  Terminal,
  Shield,
  Layers,
} from 'lucide-react';
import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { slideUpFade } from '@/design-system/motion';

export const PluginsView: React.FC = () => {
  const [activeTab, setActiveTab] = useState<'installed' | 'marketplace' | 'sdk'>('installed');

  const plugins = [
    {
      id: 'plug-1',
      name: 'VANI C++ Compiler Toolchain',
      version: '1.4.0',
      author: 'VANI Systems Team',
      description: 'Zero-overhead Clang/GCC builder and AST syntax analysis plugin for Odysseus coding agent.',
      installed: true,
      category: 'Compiler & Developer',
      downloads: '14.2k',
    },
    {
      id: 'plug-2',
      name: 'Docker Micro-Sandbox Engine',
      version: '2.1.0',
      author: 'Aegis Security Lab',
      description: 'Ephemeral micro-container runner for high-risk bash commands and untested agent scripts.',
      installed: true,
      category: 'Security & Virtualization',
      downloads: '28.9k',
    },
    {
      id: 'plug-3',
      name: 'Hugging Face Hub Model Puller',
      version: '0.9.5',
      author: 'Community Contributor',
      description: 'Direct quant downloader from Hugging Face GGUF models into Ollama runtime.',
      installed: false,
      category: 'AI & Inference',
      downloads: '8.4k',
    },
    {
      id: 'plug-4',
      name: 'Spatial Buds LE Audio Driver',
      version: '3.0.1',
      author: 'VANI Hardware Group',
      description: 'Sub-10ms dual microphone streaming driver with acoustic echo cancellation.',
      installed: true,
      category: 'Hardware',
      downloads: '42.1k',
    },
  ];

  return (
    <motion.div variants={slideUpFade} initial="initial" animate="animate" className="space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl md:text-2xl font-bold text-white tracking-tight">Plugins & Extensibility SDK</h1>
          <p className="text-xs md:text-sm text-slate-400">
            Extend VANI with custom agent capabilities, external deterministic tools, and IPC bridges.
          </p>
        </div>
        <div className="flex items-center gap-1.5 p-1 rounded-xl bg-white/[0.03] border border-white/10">
          {(['installed', 'marketplace', 'sdk'] as const).map((tab) => (
            <button
              key={tab}
              onClick={() => setActiveTab(tab)}
              className={`px-3 py-1 rounded-lg text-xs font-medium capitalize transition-all ${
                activeTab === tab ? 'bg-purple-600 text-white' : 'text-slate-400 hover:text-white'
              }`}
            >
              {tab}
            </button>
          ))}
        </div>
      </div>

      {activeTab === 'sdk' ? (
        /* SDK Documentation & Quickstart */
        <div className="space-y-6">
          <Card variant="elevated" className="space-y-4">
            <div className="flex items-center gap-2.5">
              <Code2 className="w-5 h-5 text-purple-400" />
              <h3 className="text-sm font-bold text-white">VANI Plugin SDK Specification (TypeScript / Rust / C++)</h3>
            </div>
            <p className="text-xs text-slate-300 leading-relaxed">
              Every VANI plugin exposes deterministic typed capabilities over the local IPC channel. Agents can invoke plugin tools automatically with explicit privilege boundaries.
            </p>

            <div className="rounded-xl bg-[#090d16] border border-white/10 p-4 font-mono text-xs text-purple-200 overflow-x-auto">
              <pre className="leading-relaxed">
{`import { defineVaniPlugin, ToolDefinition } from '@vani/sdk';

export default defineVaniPlugin({
  id: 'my-custom-tool',
  name: 'Database Query Tool',
  version: '1.0.0',
  permissions: ['db:read', 'network:loopback'],
  tools: [
    {
      name: 'execute_sql',
      riskLevel: 'medium',
      handler: async ({ query }) => {
        return { rows: 4, executionTimeMs: 12 };
      }
    }
  ]
});`}
              </pre>
            </div>
          </Card>
        </div>
      ) : (
        /* Installed / Marketplace List */
        <div className="grid grid-cols-1 md:grid-cols-2 gap-5">
          {plugins
            .filter((p) => (activeTab === 'installed' ? p.installed : true))
            .map((plug) => (
              <Card key={plug.id} className="flex flex-col justify-between space-y-4">
                <div className="space-y-3">
                  <div className="flex items-start justify-between">
                    <div className="flex items-center gap-3">
                      <div className="w-10 h-10 rounded-xl bg-purple-950/40 border border-purple-500/30 flex items-center justify-center text-purple-400">
                        <Puzzle className="w-5 h-5" />
                      </div>
                      <div>
                        <h3 className="text-sm font-bold text-white">{plug.name}</h3>
                        <span className="text-xs text-slate-400 font-mono">v{plug.version} • {plug.author}</span>
                      </div>
                    </div>
                    {plug.installed ? (
                      <Badge variant="success" size="xs">Installed</Badge>
                    ) : (
                      <Badge variant="outline" size="xs">{plug.downloads}</Badge>
                    )}
                  </div>

                  <p className="text-xs text-slate-300 leading-relaxed">{plug.description}</p>
                </div>

                <div className="pt-3 border-t border-white/5 flex items-center justify-between text-xs">
                  <span className="text-slate-400 font-mono text-[11px]">{plug.category}</span>
                  <Button variant={plug.installed ? 'secondary' : 'primary'} size="xs">
                    {plug.installed ? 'Configure' : 'Install Plugin'}
                  </Button>
                </div>
              </Card>
            ))}
        </div>
      )}
    </motion.div>
  );
};
