import React from 'react';
import { Outlet } from 'react-router-dom';
import { Sidebar } from './Sidebar';
import { TopBar } from './TopBar';
import { GlobalVoiceController } from './GlobalVoiceController';
import { CommandPaletteModal } from '@/components/command-palette/CommandPaletteModal';

export const RootLayout: React.FC = () => {
  return (
    <div className="flex min-h-screen bg-[#05070c] text-slate-100 font-sans selection:bg-purple-500/30 selection:text-purple-200">
      {/* Grouped Collapsible Sidebar */}
      <Sidebar />

      {/* Main Content Pane */}
      <div className="flex-1 flex flex-col min-w-0">
        <TopBar />
        <main className="flex-1 p-4 md:p-6 lg:p-8 max-w-[1720px] w-full mx-auto pb-24">
          <Outlet />
        </main>
      </div>

      {/* Global Persistent Overlays */}
      <GlobalVoiceController />
      <CommandPaletteModal />
    </div>
  );
};
