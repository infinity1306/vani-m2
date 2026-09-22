import React from 'react';
import { createBrowserRouter, Navigate } from 'react-router-dom';
import { RootLayout } from '@/layouts/RootLayout';
import { DashboardView } from '@/features/dashboard/DashboardView';
import { ChatView } from '@/features/chat/ChatView';
import { TasksView } from '@/features/tasks/TasksView';
import { AgentsView } from '@/features/agents/AgentsView';
import { MemoryView } from '@/features/memory/MemoryView';
import { ToolsView } from '@/features/tools/ToolsView';
import { IntegrationsView } from '@/features/integrations/IntegrationsView';
import { DevicesView } from '@/features/devices/DevicesView';
import { ModelsView } from '@/features/models/ModelsView';
import { SecurityView } from '@/features/security/SecurityView';
import { PluginsView } from '@/features/plugins/PluginsView';
import { DeveloperConsoleView } from '@/features/developer/DeveloperConsoleView';
import { SettingsView } from '@/features/settings/SettingsView';
import { MotionShowcaseView } from '@/features/design-system-showcase/MotionShowcaseView';

export const router = createBrowserRouter([
  {
    path: '/',
    element: <RootLayout />,
    children: [
      {
        index: true,
        element: <DashboardView />,
      },
      {
        path: 'chat',
        element: <ChatView />,
      },
      {
        path: 'tasks',
        element: <TasksView />,
      },
      {
        path: 'tasks/:id',
        element: <TasksView />,
      },
      {
        path: 'agents',
        element: <AgentsView />,
      },
      {
        path: 'agents/:id',
        element: <AgentsView />,
      },
      {
        path: 'memory',
        element: <MemoryView />,
      },
      {
        path: 'tools',
        element: <ToolsView />,
      },
      {
        path: 'integrations',
        element: <IntegrationsView />,
      },
      {
        path: 'integrations/:id',
        element: <IntegrationsView />,
      },
      {
        path: 'devices',
        element: <DevicesView />,
      },
      {
        path: 'devices/:id',
        element: <DevicesView />,
      },
      {
        path: 'models',
        element: <ModelsView />,
      },
      {
        path: 'security',
        element: <SecurityView />,
      },
      {
        path: 'plugins',
        element: <PluginsView />,
      },
      {
        path: 'developer',
        element: <DeveloperConsoleView />,
      },
      {
        path: 'settings',
        element: <SettingsView />,
      },
      {
        path: 'design-system',
        element: <MotionShowcaseView />,
      },
      {
        path: '*',
        element: <Navigate to="/" replace />,
      },
    ],
  },
]);
