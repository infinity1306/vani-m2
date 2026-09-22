import React from 'react';
import { NavLink, useLocation } from 'react-router-dom';
import {
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
  Puzzle,
  Terminal,
  Settings,
  ChevronLeft,
  ChevronRight,
  Sparkles,
} from 'lucide-react';
import { useVaniStore } from '@/stores/useVaniStore';

interface NavItem {
  path: string;
  label: string;
  icon: React.ReactNode;
  badge?: string | number;
}

export const Sidebar: React.FC = () => {
  const isCollapsed = useVaniStore((s) => s.isSidebarCollapsed);
  const toggleSidebar = useVaniStore((s) => s.toggleSidebar);
  const tasks = useVaniStore((s) => s.tasks);
  const permissions = useVaniStore((s) => s.permissions);
  const isOffline = useVaniStore((s) => s.isOffline);

  const runningTasksCount = tasks.filter((t) => t.state === 'running').length;
  const pendingPermsCount = permissions.filter((p) => p.status === 'pending').length;

  const coreNav: NavItem[] = [
    { path: '/', label: 'Dashboard', icon: <LayoutDashboard className="w-4 h-4" /> },
    { path: '/chat', label: 'Chat & Voice', icon: <MessageSquare className="w-4 h-4" /> },
    { path: '/tasks', label: 'Tasks', icon: <CheckSquare className="w-4 h-4" />, badge: runningTasksCount > 0 ? runningTasksCount : undefined },
    { path: '/agents', label: 'Agents', icon: <Bot className="w-4 h-4" /> },
    { path: '/memory', label: 'Memory', icon: <Brain className="w-4 h-4" /> },
    { path: '/tools', label: 'Tools', icon: <Wrench className="w-4 h-4" /> },
    { path: '/integrations', label: 'Integrations', icon: <Layers className="w-4 h-4" /> },
    { path: '/devices', label: 'Devices', icon: <Smartphone className="w-4 h-4" /> },
    { path: '/models', label: 'Models', icon: <Cpu className="w-4 h-4" /> },
    { path: '/security', label: 'Security', icon: <Shield className="w-4 h-4" />, badge: pendingPermsCount > 0 ? pendingPermsCount : undefined },
  ];

  const devNav: NavItem[] = [
    { path: '/plugins', label: 'Plugins & SDK', icon: <Puzzle className="w-4 h-4" /> },
    { path: '/developer', label: 'Developer Console', icon: <Terminal className="w-4 h-4" /> },
    { path: '/design-system', label: 'Motion Showcase', icon: <Sparkles className="w-4 h-4" /> },
  ];

  return (
    <aside
      className={`h-screen sticky top-0 bg-[#060911] border-r border-white/[0.08] flex flex-col justify-between transition-all duration-300 z-50 select-none ${
        isCollapsed ? 'w-16' : 'w-60'
      }`}
    >
      {/* Top Brand Area */}
      <div>
        <div className="h-14 border-b border-white/[0.08] px-4 flex items-center justify-between">
          <div className="flex items-center gap-3 overflow-hidden">
            {/* VANI Logo Orb */}
            <div className="w-8 h-8 rounded-xl bg-gradient-to-tr from-purple-600 to-cyan-500 p-[1px] flex-shrink-0 shadow-[0_0_15px_rgba(139,92,246,0.4)]">
              <div className="w-full h-full bg-[#090d16] rounded-[11px] flex items-center justify-center">
                <span className="text-purple-400 font-extrabold text-xs tracking-tighter">V2</span>
              </div>
            </div>
            {!isCollapsed && (
              <div className="flex flex-col min-w-0">
                <span className="text-sm font-bold tracking-tight text-white flex items-center gap-1.5">
                  VANI <span className="text-xs px-1.5 py-0.2 rounded bg-purple-950/80 text-purple-300 font-mono border border-purple-500/30">M2</span>
                </span>
                <span className="text-[10px] text-slate-400 truncate">Your AI. Your OS. Your Way.</span>
              </div>
            )}
          </div>
          <button
            onClick={toggleSidebar}
            className="text-slate-500 hover:text-slate-300 p-1 rounded-md hover:bg-white/[0.06] transition-colors"
            title={isCollapsed ? 'Expand sidebar' : 'Collapse sidebar'}
          >
            {isCollapsed ? <ChevronRight className="w-4 h-4" /> : <ChevronLeft className="w-4 h-4" />}
          </button>
        </div>

        {/* Navigation Links */}
        <div className="p-2 space-y-6 overflow-y-auto max-h-[calc(100vh-8rem)]">
          {/* Core System Group */}
          <div>
            {!isCollapsed && (
              <div className="px-3 pb-1.5 text-[10px] uppercase font-mono tracking-wider text-slate-500 font-semibold">
                Operating Core
              </div>
            )}
            <nav className="space-y-0.5">
              {coreNav.map((item) => (
                <NavLink
                  key={item.path}
                  to={item.path}
                  title={isCollapsed ? item.label : undefined}
                  className={({ isActive }) =>
                    `flex items-center gap-3 px-3 py-2 rounded-xl text-xs font-medium transition-all ${
                      isActive
                        ? 'bg-purple-600/20 text-white border border-purple-500/40 shadow-sm shadow-purple-900/20'
                        : 'text-slate-400 hover:text-slate-200 hover:bg-white/[0.04]'
                    } ${isCollapsed ? 'justify-center px-0' : ''}`
                  }
                >
                  <span className="flex-shrink-0">{item.icon}</span>
                  {!isCollapsed && (
                    <span className="flex-1 truncate">{item.label}</span>
                  )}
                  {!isCollapsed && item.badge && (
                    <span className="text-[10px] font-mono px-1.5 py-0.2 rounded-full bg-purple-500/30 text-purple-300 border border-purple-400/30">
                      {item.badge}
                    </span>
                  )}
                </NavLink>
              ))}
            </nav>
          </div>

          {/* Developer & Extensibility Group */}
          <div>
            {!isCollapsed && (
              <div className="px-3 pb-1.5 text-[10px] uppercase font-mono tracking-wider text-slate-500 font-semibold">
                Ecosystem & Dev
              </div>
            )}
            <nav className="space-y-0.5">
              {devNav.map((item) => (
                <NavLink
                  key={item.path}
                  to={item.path}
                  title={isCollapsed ? item.label : undefined}
                  className={({ isActive }) =>
                    `flex items-center gap-3 px-3 py-2 rounded-xl text-xs font-medium transition-all ${
                      isActive
                        ? 'bg-purple-600/20 text-white border border-purple-500/40 shadow-sm shadow-purple-900/20'
                        : 'text-slate-400 hover:text-slate-200 hover:bg-white/[0.04]'
                    } ${isCollapsed ? 'justify-center px-0' : ''}`
                  }
                >
                  <span className="flex-shrink-0">{item.icon}</span>
                  {!isCollapsed && <span className="flex-1 truncate">{item.label}</span>}
                </NavLink>
              ))}
            </nav>
          </div>
        </div>
      </div>

      {/* Bottom Footer Area */}
      <div className="p-2 border-t border-white/[0.08] space-y-1">
        <NavLink
          to="/settings"
          title={isCollapsed ? 'Settings' : undefined}
          className={({ isActive }) =>
            `flex items-center gap-3 px-3 py-2 rounded-xl text-xs font-medium transition-all ${
              isActive
                ? 'bg-purple-600/20 text-white border border-purple-500/40'
                : 'text-slate-400 hover:text-slate-200 hover:bg-white/[0.04]'
            } ${isCollapsed ? 'justify-center px-0' : ''}`
          }
        >
          <Settings className="w-4 h-4 flex-shrink-0" />
          {!isCollapsed && <span className="flex-1 truncate">Settings</span>}
        </NavLink>

        {/* Connectivity status pill */}
        {!isCollapsed && (
          <div className="px-3 py-2 rounded-xl bg-white/[0.02] border border-white/5 flex items-center justify-between">
            <div className="flex items-center gap-2">
              <span className={`w-2 h-2 rounded-full ${isOffline ? 'bg-amber-400' : 'bg-emerald-400 shadow-[0_0_8px_#10b981]'}`} />
              <span className="text-[11px] font-medium text-slate-300 font-mono">
                {isOffline ? 'Local Engine' : 'VANI Online'}
              </span>
            </div>
            <span className="text-[10px] text-slate-500 font-mono">v2.4</span>
          </div>
        )}
      </div>
    </aside>
  );
};
