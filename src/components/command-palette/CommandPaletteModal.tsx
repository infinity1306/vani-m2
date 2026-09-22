import React, { useState, useEffect, useMemo } from 'react';
import { useNavigate } from 'react-router-dom';
import { motion, AnimatePresence } from 'framer-motion';
import {
  Search,
  LayoutDashboard,
  MessageSquare,
  CheckSquare,
  Bot,
  Brain,
  Wrench,
  Layers,
  Smartphone,
  Cpu,
  Shield,
  Terminal,
  Camera,
  Play,
  Moon,
  WifiOff,
  Sparkles,
  ArrowRight,
  Code2,
} from 'lucide-react';
import { useVaniStore } from '@/stores/useVaniStore';
import { useVoiceStore } from '@/stores/useVoiceStore';

interface CommandItem {
  id: string;
  title: string;
  category: 'Navigation' | 'Actions' | 'Tasks' | 'Agents' | 'Tools';
  icon: React.ReactNode;
  shortcut?: string;
  action: () => void;
}

export const CommandPaletteModal: React.FC = () => {
  const isOpen = useVaniStore((s) => s.isCommandPaletteOpen);
  const setIsOpen = useVaniStore((s) => s.setCommandPaletteOpen);
  const tasks = useVaniStore((s) => s.tasks);
  const agents = useVaniStore((s) => s.agents);
  const toggleOffline = useVaniStore((s) => s.toggleOffline);
  const isOffline = useVaniStore((s) => s.isOffline);
  const triggerVoicePrompt = useVoiceStore((s) => s.triggerVoicePrompt);

  const [query, setQuery] = useState('');
  const [selectedIndex, setSelectedIndex] = useState(0);
  const navigate = useNavigate();

  // Keyboard shortcut listener
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if ((e.metaKey || e.ctrlKey) && e.key === 'k') {
        e.preventDefault();
        setIsOpen(!isOpen);
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [isOpen, setIsOpen]);

  const commands: CommandItem[] = useMemo(() => [
    // Navigation
    { id: 'nav-dash', title: 'Go to Dashboard', category: 'Navigation', icon: <LayoutDashboard className="w-4 h-4 text-purple-400" />, action: () => navigate('/') },
    { id: 'nav-chat', title: 'Go to Chat & Voice Canvas', category: 'Navigation', icon: <MessageSquare className="w-4 h-4 text-cyan-400" />, action: () => navigate('/chat') },
    { id: 'nav-tasks', title: 'Go to Task Center', category: 'Navigation', icon: <CheckSquare className="w-4 h-4 text-indigo-400" />, action: () => navigate('/tasks') },
    { id: 'nav-agents', title: 'Go to Agents Control', category: 'Navigation', icon: <Bot className="w-4 h-4 text-emerald-400" />, action: () => navigate('/agents') },
    { id: 'nav-memory', title: 'Go to Memory Explorer', category: 'Navigation', icon: <Brain className="w-4 h-4 text-pink-400" />, action: () => navigate('/memory') },
    { id: 'nav-tools', title: 'Go to Tool Registry', category: 'Navigation', icon: <Wrench className="w-4 h-4 text-amber-400" />, action: () => navigate('/tools') },
    { id: 'nav-integrations', title: 'Go to Integrations (CRM/LMS/GitHub)', category: 'Navigation', icon: <Layers className="w-4 h-4 text-blue-400" />, action: () => navigate('/integrations') },
    { id: 'nav-devices', title: 'Go to Connected Devices', category: 'Navigation', icon: <Smartphone className="w-4 h-4 text-teal-400" />, action: () => navigate('/devices') },
    { id: 'nav-models', title: 'Go to Models & Providers', category: 'Navigation', icon: <Cpu className="w-4 h-4 text-violet-400" />, action: () => navigate('/models') },
    { id: 'nav-security', title: 'Go to Security & Audit Center', category: 'Navigation', icon: <Shield className="w-4 h-4 text-rose-400" />, action: () => navigate('/security') },
    { id: 'nav-dev', title: 'Go to Developer Console & IPC Logs', category: 'Navigation', icon: <Terminal className="w-4 h-4 text-slate-400" />, action: () => navigate('/developer') },
    { id: 'nav-design', title: 'Go to Motion & Transitions Showcase', category: 'Navigation', icon: <Sparkles className="w-4 h-4 text-yellow-400" />, action: () => navigate('/design-system') },

    // Quick Actions
    {
      id: 'act-voice-coding',
      title: 'Run Voice: "mera react wala project run karde"',
      category: 'Actions',
      icon: <Sparkles className="w-4 h-4 text-cyan-400" />,
      action: () => triggerVoicePrompt('mera react wala project run karke auth check karde'),
    },
    {
      id: 'act-offline-toggle',
      title: isOffline ? 'Switch to Online Mode (Enable Cloud)' : 'Switch to Local-Only Mode (Offline)',
      category: 'Actions',
      icon: <WifiOff className="w-4 h-4 text-amber-400" />,
      action: () => toggleOffline(),
    },
    {
      id: 'act-screenshot',
      title: 'Capture Desktop Screenshot with Privacy Mask',
      category: 'Actions',
      icon: <Camera className="w-4 h-4 text-emerald-400" />,
      action: () => {
        navigate('/tools');
      },
    },

    // Running Tasks
    ...tasks.map((task) => ({
      id: `task-${task.id}`,
      title: `Inspect Task: ${task.title}`,
      category: 'Tasks' as const,
      icon: <CheckSquare className="w-4 h-4 text-purple-400" />,
      action: () => navigate(`/tasks/${task.id}`),
    })),

    // Agents
    ...agents.map((agent) => ({
      id: `agent-${agent.id}`,
      title: `Agent: ${agent.name} (${agent.role})`,
      category: 'Agents' as const,
      icon: <Bot className="w-4 h-4 text-cyan-400" />,
      action: () => navigate(`/agents/${agent.id}`),
    })),
  ], [navigate, isOffline, toggleOffline, triggerVoicePrompt, tasks, agents]);

  const filteredCommands = useMemo(() => {
    if (!query.trim()) return commands;
    const q = query.toLowerCase();
    return commands.filter(
      (cmd) =>
        cmd.title.toLowerCase().includes(q) ||
        cmd.category.toLowerCase().includes(q)
    );
  }, [commands, query]);

  useEffect(() => {
    setSelectedIndex(0);
  }, [query]);

  const executeCommand = (cmd: CommandItem) => {
    cmd.action();
    setIsOpen(false);
    setQuery('');
  };

  const handleKeyDownModal = (e: React.KeyboardEvent) => {
    if (e.key === 'ArrowDown') {
      e.preventDefault();
      setSelectedIndex((prev) => (prev + 1) % filteredCommands.length);
    } else if (e.key === 'ArrowUp') {
      e.preventDefault();
      setSelectedIndex((prev) => (prev - 1 + filteredCommands.length) % filteredCommands.length);
    } else if (e.key === 'Enter' && filteredCommands[selectedIndex]) {
      e.preventDefault();
      executeCommand(filteredCommands[selectedIndex]);
    }
  };

  return (
    <AnimatePresence>
      {isOpen && (
        <div className="fixed inset-0 z-[150] flex items-start justify-center pt-20 px-4">
          {/* Backdrop */}
          <motion.div
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            onClick={() => setIsOpen(false)}
            className="fixed inset-0 bg-black/80 backdrop-blur-md"
          />

          {/* Palette Box */}
          <motion.div
            initial={{ opacity: 0, scale: 0.95, y: -10 }}
            animate={{ opacity: 1, scale: 1, y: 0 }}
            exit={{ opacity: 0, scale: 0.95, y: -10 }}
            transition={{ duration: 0.18, ease: [0.16, 1, 0.3, 1] }}
            className="relative w-full max-w-2xl bg-[#090d16] border border-white/15 rounded-2xl shadow-2xl overflow-hidden flex flex-col z-10"
            onKeyDown={handleKeyDownModal}
          >
            {/* Search Input Bar */}
            <div className="flex items-center gap-3 px-5 py-4 border-b border-white/10 bg-white/[0.02]">
              <Search className="w-5 h-5 text-purple-400 flex-shrink-0" />
              <input
                type="text"
                autoFocus
                value={query}
                onChange={(e) => setQuery(e.target.value)}
                placeholder="Type a command, ask VANI, search tasks, agents, tools..."
                className="w-full bg-transparent text-white placeholder:text-slate-500 text-sm md:text-base outline-none font-medium"
              />
              <span className="text-[11px] px-2 py-0.5 rounded bg-white/10 text-slate-400 font-mono">
                ESC
              </span>
            </div>

            {/* Results List */}
            <div className="max-h-96 overflow-y-auto p-2 space-y-1">
              {filteredCommands.length === 0 ? (
                <div className="py-12 text-center text-slate-400 text-sm">
                  No matching system commands or entities found for "{query}".
                </div>
              ) : (
                filteredCommands.map((cmd, idx) => {
                  const isSelected = idx === selectedIndex;
                  return (
                    <div
                      key={cmd.id}
                      onClick={() => executeCommand(cmd)}
                      onMouseEnter={() => setSelectedIndex(idx)}
                      className={`flex items-center justify-between px-3.5 py-2.5 rounded-xl text-sm transition-colors cursor-pointer ${
                        isSelected
                          ? 'bg-purple-600/20 border border-purple-500/40 text-white'
                          : 'text-slate-300 hover:bg-white/[0.04] border border-transparent'
                      }`}
                    >
                      <div className="flex items-center gap-3 min-w-0">
                        <div className="p-1.5 rounded-lg bg-white/[0.06] flex-shrink-0">
                          {cmd.icon}
                        </div>
                        <span className="truncate font-medium">{cmd.title}</span>
                      </div>
                      <div className="flex items-center gap-2 flex-shrink-0">
                        <span className="text-[10px] uppercase font-mono px-2 py-0.5 rounded bg-white/[0.06] text-slate-400">
                          {cmd.category}
                        </span>
                        {isSelected && (
                          <ArrowRight className="w-3.5 h-3.5 text-purple-400" />
                        )}
                      </div>
                    </div>
                  );
                })
              )}
            </div>

            {/* Palette Footer */}
            <div className="flex items-center justify-between px-5 py-2.5 bg-black/40 border-t border-white/5 text-[11px] text-slate-500">
              <div className="flex items-center gap-4">
                <span>Navigate <kbd className="px-1 py-0.5 rounded bg-white/10 text-slate-300">↑↓</kbd></span>
                <span>Select <kbd className="px-1 py-0.5 rounded bg-white/10 text-slate-300">↵</kbd></span>
              </div>
              <div className="flex items-center gap-1.5 text-purple-400/80">
                <Sparkles className="w-3 h-3" />
                <span>VANI Mark 2 Command Layer</span>
              </div>
            </div>
          </motion.div>
        </div>
      )}
    </AnimatePresence>
  );
};
