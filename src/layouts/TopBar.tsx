import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import {
  Search,
  Mic,
  Cpu,
  Wifi,
  WifiOff,
  Bell,
  CheckCircle2,
  Sliders,
  Sparkles,
  User,
  Shield,
  Layers,
} from 'lucide-react';
import { useVaniStore } from '@/stores/useVaniStore';
import { useVoiceStore } from '@/stores/useVoiceStore';
import { Badge } from '@/components/ui/Badge';
import { NotificationDrawer } from '@/components/notifications/NotificationDrawer';

export const TopBar: React.FC = () => {
  const setCommandPaletteOpen = useVaniStore((s) => s.setCommandPaletteOpen);
  const isOffline = useVaniStore((s) => s.isOffline);
  const isBackendConnected = useVaniStore((s) => s.isBackendConnected);
  const toggleOffline = useVaniStore((s) => s.toggleOffline);
  const tasks = useVaniStore((s) => s.tasks);
  const notifications = useVaniStore((s) => s.notifications);
  const models = useVaniStore((s) => s.models);

  const voiceState = useVoiceStore((s) => s.state);
  const isMicActive = useVoiceStore((s) => s.isMicActive);
  const setMicActive = useVoiceStore((s) => s.setMicActive);

  const [isNotifOpen, setIsNotifOpen] = useState(false);
  const navigate = useNavigate();

  const activeTasksCount = tasks.filter((t) => t.state === 'running').length;
  const unreadNotifsCount = notifications.filter((n) => !n.read).length;
  const activeModel = models.find((m) => m.status === 'active') || models[0];

  const getVoiceChip = () => {
    switch (voiceState) {
      case 'listening':
        return { label: 'Listening...', color: 'bg-cyan-500/20 text-cyan-300 border-cyan-500/40 animate-pulse' };
      case 'thinking':
        return { label: 'Thinking...', color: 'bg-purple-500/20 text-purple-300 border-purple-500/40' };
      case 'executing':
        return { label: 'Executing...', color: 'bg-indigo-500/20 text-indigo-300 border-indigo-500/40' };
      case 'speaking':
        return { label: 'Speaking...', color: 'bg-purple-500/30 text-purple-200 border-purple-400' };
      default:
        return { label: 'Ready', color: 'bg-white/[0.04] text-slate-400 border-white/10' };
    }
  };

  const voiceChip = getVoiceChip();

  return (
    <>
      <header className="h-14 border-b border-white/[0.08] bg-[#070a12]/80 backdrop-blur-xl px-4 md:px-6 flex items-center justify-between sticky top-0 z-40 select-none">
        {/* Left: Command Search Bar */}
        <div className="flex items-center gap-3">
          <button
            onClick={() => setCommandPaletteOpen(true)}
            className="flex items-center gap-2.5 px-3 py-1.5 rounded-xl bg-white/[0.04] hover:bg-white/[0.08] border border-white/10 text-slate-400 hover:text-slate-200 text-xs md:text-sm transition-all w-52 md:w-80 justify-between group"
          >
            <div className="flex items-center gap-2">
              <Search className="w-3.5 h-3.5 text-purple-400 group-hover:scale-110 transition-transform" />
              <span className="font-medium text-slate-400 group-hover:text-slate-200">Search anything...</span>
            </div>
            <kbd className="hidden sm:inline-flex items-center gap-0.5 text-[10px] font-mono px-1.5 py-0.5 rounded bg-white/10 text-slate-400">
              ⌘K
            </kbd>
          </button>
        </div>

        {/* Right: Quick Status Chips & User Controls */}
        <div className="flex items-center gap-2 md:gap-3">
          {/* Live Backend Core Connection Badge */}
          <div
            onClick={() => navigate('/developer')}
            title={isBackendConnected ? 'Connected to VANI C++ Runtime Core Gateway (Port 3002)' : 'Local Core Standalone Mode'}
            className={`hidden sm:flex items-center gap-1.5 px-2.5 py-1 rounded-lg border text-xs font-mono transition-colors cursor-pointer ${
              isBackendConnected
                ? 'bg-cyan-950/40 border-cyan-500/40 text-cyan-300'
                : 'bg-white/[0.03] border-white/10 text-slate-400'
            }`}
          >
            <span className={`w-1.5 h-1.5 rounded-full ${isBackendConnected ? 'bg-cyan-400 animate-pulse' : 'bg-slate-500'}`} />
            <span>{isBackendConnected ? 'Backend Live' : 'Local Core'}</span>
          </div>

          {/* Voice State Quick Chip */}
          <button
            onClick={() => setMicActive(!isMicActive)}
            className={`flex items-center gap-1.5 px-2.5 py-1 rounded-lg border text-xs font-medium transition-all ${voiceChip.color}`}
          >
            <Mic className={`w-3.5 h-3.5 ${isMicActive ? 'text-cyan-400 animate-bounce' : 'text-slate-400'}`} />
            <span className="hidden sm:inline">{voiceChip.label}</span>
          </button>

          {/* Model Status Chip */}
          <div
            onClick={() => navigate('/models')}
            className="hidden md:flex items-center gap-1.5 px-2.5 py-1 rounded-lg bg-white/[0.03] border border-white/10 text-xs text-slate-300 hover:border-purple-500/40 cursor-pointer transition-colors"
          >
            <Cpu className="w-3.5 h-3.5 text-purple-400" />
            <span className="font-mono text-[11px] font-medium">{activeModel.name.split(' ')[0]}</span>
            <span className="text-[10px] text-purple-400 bg-purple-950/50 px-1 rounded">
              {activeModel.type === 'local' ? 'Local' : 'Cloud'}
            </span>
          </div>

          {/* Online / Local-First Mode Switch */}
          <button
            onClick={toggleOffline}
            title={isOffline ? 'Offline Mode Active (Local Models Only)' : 'Online Mesh Active (Cloud & Local)'}
            className={`flex items-center gap-1.5 px-2.5 py-1 rounded-lg border text-xs font-medium transition-all ${
              isOffline
                ? 'bg-amber-950/30 border-amber-500/40 text-amber-300'
                : 'bg-emerald-950/30 border-emerald-500/40 text-emerald-300'
            }`}
          >
            {isOffline ? <WifiOff className="w-3.5 h-3.5" /> : <Wifi className="w-3.5 h-3.5" />}
            <span className="hidden sm:inline">{isOffline ? 'Offline (Local)' : 'Online'}</span>
          </button>

          {/* Running Tasks Counter */}
          <button
            onClick={() => navigate('/tasks')}
            className="flex items-center gap-1.5 px-2.5 py-1 rounded-lg bg-white/[0.04] border border-white/10 text-xs text-slate-300 hover:bg-white/[0.08] transition-colors"
          >
            <span className="w-2 h-2 rounded-full bg-purple-400 animate-ping" />
            <span className="font-mono text-xs font-semibold">{activeTasksCount}</span>
            <span className="hidden lg:inline text-slate-400">Tasks</span>
          </button>

          {/* Notification Bell */}
          <button
            onClick={() => setIsNotifOpen(true)}
            className="relative p-2 rounded-lg bg-white/[0.04] hover:bg-white/[0.08] border border-white/10 text-slate-300 hover:text-white transition-colors"
          >
            <Bell className="w-4 h-4" />
            {unreadNotifsCount > 0 && (
              <span className="absolute top-1 right-1 w-2 h-2 rounded-full bg-cyan-400 shadow-[0_0_6px_#06b6d4]" />
            )}
          </button>

          {/* User Profile avatar */}
          <button
            onClick={() => navigate('/settings')}
            className="flex items-center gap-2 pl-1 pr-2 py-1 rounded-lg bg-white/[0.04] hover:bg-white/[0.08] border border-white/10 text-slate-200 transition-colors"
          >
            <div className="w-6 h-6 rounded-full bg-gradient-to-tr from-purple-600 to-indigo-600 flex items-center justify-center text-[10px] font-bold text-white shadow-inner">
              AS
            </div>
            <span className="hidden xl:inline text-xs font-medium">Anant</span>
          </button>
        </div>
      </header>

      {/* Notifications Drawer */}
      <NotificationDrawer isOpen={isNotifOpen} onClose={() => setIsNotifOpen(false)} />
    </>
  );
};
