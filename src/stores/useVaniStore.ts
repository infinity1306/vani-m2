import { create } from 'zustand';
import {
  Task,
  Agent,
  Tool,
  Model,
  MemoryItem,
  Integration,
  ConnectedDevice,
  PermissionRequest,
  SystemMetric,
  ActivityEvent,
  NotificationItem,
  ChatMessage,
} from '@/types';
import {
  initialTasks,
  initialAgents,
  initialTools,
  initialModels,
  initialMemory,
  initialIntegrations,
  initialDevices,
  initialPermissions,
  initialSystemMetric,
  initialActivity,
  initialNotifications,
  initialChatMessages,
} from '@/services/mocks/data';
import { eventBus } from '@/services/runtime/eventBus';
import { vaniApiClient } from '@/services/runtime/vaniApiClient';
import { speechService } from '@/services/voice/speechService';
import { conversationEngine } from '@/services/ai/conversationEngine';

interface VaniStoreState {
  tasks: Task[];
  agents: Agent[];
  tools: Tool[];
  models: Model[];
  memory: MemoryItem[];
  integrations: Integration[];
  devices: ConnectedDevice[];
  permissions: PermissionRequest[];
  systemMetrics: SystemMetric;
  activity: ActivityEvent[];
  notifications: NotificationItem[];
  chatMessages: ChatMessage[];
  isOffline: boolean;
  isBackendConnected: boolean;
  isSidebarCollapsed: boolean;
  isCommandPaletteOpen: boolean;
  selectedTaskId: string | null;
  selectedAgentId: string | null;

  // Actions
  toggleOffline: () => void;
  toggleSidebar: () => void;
  setSidebarCollapsed: (collapsed: boolean) => void;
  setCommandPaletteOpen: (open: boolean) => void;
  setSelectedTaskId: (id: string | null) => void;
  setSelectedAgentId: (id: string | null) => void;

  // Entity Actions
  addTask: (task: Omit<Task, 'id'>) => string;
  updateTask: (id: string, updates: Partial<Task>) => void;
  cancelTask: (id: string) => void;
  retryTask: (id: string) => void;

  allowPermission: (id: string) => void;
  denyPermission: (id: string) => void;

  forgetMemory: (id: string) => void;
  addMemory: (item: Omit<MemoryItem, 'id' | 'createdAt' | 'lastAccessed'>) => void;
  togglePinMemory: (id: string) => void;

  setActiveModel: (id: string) => void;
  toggleToolStatus: (id: string) => void;
  toggleIntegrationConnection: (id: string) => void;
  toggleDeviceConnection: (id: string) => void;

  sendChatMessage: (text: string, isVoice?: boolean) => void;
  markNotificationAsRead: (id: string) => void;
  clearAllNotifications: () => void;
}

export const useVaniStore = create<VaniStoreState>((set, get) => {
  // Real-time Event bus listeners
  eventBus.on<ActivityEvent>('activity:created', (newActivity) => {
    set((state) => ({ activity: [newActivity, ...state.activity.slice(0, 30)] }));
  });

  eventBus.on<NotificationItem>('notification:created', (notif) => {
    set((state) => ({ notifications: [notif, ...state.notifications] }));
  });

  eventBus.on<{ connected: boolean }>('runtime:connection:changed', ({ connected }) => {
    set({ isBackendConnected: connected });
  });

  eventBus.on<SystemMetric>('system:metrics:updated', (metrics) => {
    set((state) => ({ systemMetrics: { ...state.systemMetrics, ...metrics } }));
  });

  eventBus.on<Task>('task:created', (task) => {
    set((state) => {
      if (state.tasks.some((t) => t.id === task.id)) return state;
      return { tasks: [task, ...state.tasks] };
    });
  });

  eventBus.on<Task>('task:updated', (updated) => {
    set((state) => ({
      tasks: state.tasks.map((t) => (t.id === updated.id ? { ...t, ...updated } : t)),
    }));
  });

  eventBus.on<Task>('task:completed', (completed) => {
    set((state) => ({
      tasks: state.tasks.map((t) => (t.id === completed.id ? { ...t, ...completed } : t)),
      notifications: [
        {
          id: `notif-${Date.now()}`,
          title: 'Task Finished',
          message: `${completed.title} completed successfully.`,
          timestamp: 'Just now',
          type: 'success',
          read: false,
        },
        ...state.notifications,
      ],
    }));
  });

  // Initial fetch from backend if running
  vaniApiClient.getStatus().then((status) => {
    if (status) {
      set({ isBackendConnected: true });
      if (status.metrics) {
        set((state) => ({ systemMetrics: { ...state.systemMetrics, ...status.metrics } }));
      }
    }
  });

  return {
    tasks: initialTasks,
    agents: initialAgents,
    tools: initialTools,
    models: initialModels,
    memory: initialMemory,
    integrations: initialIntegrations,
    devices: initialDevices,
    permissions: initialPermissions,
    systemMetrics: initialSystemMetric,
    activity: initialActivity,
    notifications: initialNotifications,
    chatMessages: initialChatMessages,
    isOffline: false,
    isBackendConnected: false,
    isSidebarCollapsed: false,
    isCommandPaletteOpen: false,
    selectedTaskId: null,
    selectedAgentId: null,

    toggleOffline: () => {
      set((state) => {
        const newOffline = !state.isOffline;
        eventBus.emit('runtime:connectivity:changed', { isOffline: newOffline });
        eventBus.emit('activity:created', {
          id: `act-${Date.now()}`,
          timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
          action: newOffline ? 'Switched to Local-Only (Offline Mode)' : 'Switched to Cloud + Local Mesh (Online Mode)',
          target: 'Connectivity Engine',
          type: 'system',
        });
        return { isOffline: newOffline };
      });
    },

    toggleSidebar: () => set((state) => ({ isSidebarCollapsed: !state.isSidebarCollapsed })),
    setSidebarCollapsed: (isSidebarCollapsed) => set({ isSidebarCollapsed }),
    setCommandPaletteOpen: (isCommandPaletteOpen) => set({ isCommandPaletteOpen }),
    setSelectedTaskId: (selectedTaskId) => set({ selectedTaskId }),
    setSelectedAgentId: (selectedAgentId) => set({ selectedAgentId }),

    addTask: (taskData) => {
      const newId = `task-${Date.now()}`;
      const newTask: Task = {
        ...taskData,
        id: newId,
        state: 'running',
        progress: 10,
        startedAt: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
      };
      set((state) => ({ tasks: [newTask, ...state.tasks] }));
      eventBus.emit('activity:created', {
        id: `act-${Date.now()}`,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
        action: `Task created: ${newTask.title}`,
        target: newTask.agentName || 'VANI Core',
        type: 'task',
      });

      // Synchronize with backend runtime if connected
      vaniApiClient.createTask(newTask).catch(() => {});

      return newId;
    },

    updateTask: (id, updates) => {
      set((state) => ({
        tasks: state.tasks.map((t) => (t.id === id ? { ...t, ...updates } : t)),
      }));
    },

    cancelTask: (id) => {
      set((state) => ({
        tasks: state.tasks.map((t) => (t.id === id ? { ...t, state: 'cancelled' } : t)),
      }));
      eventBus.emit('activity:created', {
        id: `act-${Date.now()}`,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
        action: 'Cancelled task execution',
        target: id,
        type: 'task',
      });
    },

    retryTask: (id) => {
      set((state) => ({
        tasks: state.tasks.map((t) => (t.id === id ? { ...t, state: 'running', progress: 15 } : t)),
      }));
    },

    allowPermission: (id) => {
      set((state) => ({
        permissions: state.permissions.map((p) => (p.id === id ? { ...p, status: 'allowed' } : p)),
      }));
      eventBus.emit('activity:created', {
        id: `act-${Date.now()}`,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
        action: 'Permission Approved by User',
        target: id,
        type: 'security',
      });
    },

    denyPermission: (id) => {
      set((state) => ({
        permissions: state.permissions.map((p) => (p.id === id ? { ...p, status: 'denied' } : p)),
      }));
      eventBus.emit('activity:created', {
        id: `act-${Date.now()}`,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
        action: 'Permission Denied by User',
        target: id,
        type: 'security',
      });
    },

    forgetMemory: (id) => {
      set((state) => ({
        memory: state.memory.filter((m) => m.id !== id),
      }));
      eventBus.emit('activity:created', {
        id: `act-${Date.now()}`,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
        action: 'Memory forgotten (Permanently purged)',
        target: id,
        type: 'system',
      });
    },

    addMemory: (item) => {
      const newMemory: MemoryItem = {
        ...item,
        id: `mem-${Date.now()}`,
        createdAt: 'Just now',
        lastAccessed: 'Just now',
      };
      set((state) => ({ memory: [newMemory, ...state.memory] }));
    },

    togglePinMemory: (id) => {
      set((state) => ({
        memory: state.memory.map((m) => (m.id === id ? { ...m, isPinned: !m.isPinned } : m)),
      }));
    },

    setActiveModel: (id) => {
      set((state) => ({
        models: state.models.map((m) => ({
          ...m,
          status: m.id === id ? 'active' : m.status === 'active' ? 'available' : m.status,
        })),
      }));
      eventBus.emit('activity:created', {
        id: `act-${Date.now()}`,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
        action: 'Switched primary model',
        target: id,
        type: 'system',
      });
    },

    toggleToolStatus: (id) => {
      set((state) => ({
        tools: state.tools.map((t) =>
          t.id === id ? { ...t, status: t.status === 'active' ? 'disabled' : 'active' } : t
        ),
      }));
    },

    toggleIntegrationConnection: (id) => {
      set((state) => ({
        integrations: state.integrations.map((i) =>
          i.id === id
            ? { ...i, status: i.status === 'connected' ? 'disconnected' : 'connected' }
            : i
        ),
      }));
    },

    toggleDeviceConnection: (id) => {
      set((state) => ({
        devices: state.devices.map((d) =>
          d.id === id
            ? { ...d, status: d.status === 'connected' ? 'disconnected' : 'connected' }
            : d
        ),
      }));
    },

    sendChatMessage: async (text, isVoice = false) => {
      const userMsg: ChatMessage = {
        id: `msg-${Date.now()}`,
        sender: 'user',
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
        text,
        voiceUsed: isVoice,
        language: text.toLowerCase().includes('karde') || text.toLowerCase().includes('mera') || text.toLowerCase().includes('hai') ? 'Hinglish' : 'English',
      };

      set((state) => ({ chatMessages: [...state.chatMessages, userMsg] }));

      // Generate intelligent conversational AI response
      const state = get();
      const aiResponse = await conversationEngine.generateResponse(text, {
        activeTasksCount: state.tasks.filter((t) => t.state === 'running').length,
        cpuUsage: state.systemMetrics?.cpuUsage ?? 18,
        memoryUsage: state.systemMetrics ? Math.round((state.systemMetrics.ramUsedGb / state.systemMetrics.ramTotalGb) * 100) : 42,
        isOffline: state.isOffline,
        activeModel: state.models.find((m) => m.status === 'active')?.name || 'Llama 3.3 70B (Local)',
      });

      const vaniMsg: ChatMessage = {
        id: `msg-${Date.now() + 1}`,
        sender: 'vani',
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
        text: aiResponse.replyText,
        agentCard: aiResponse.agentCard,
        toolsUsed: aiResponse.toolsUsed,
        actionButtons: [
          { id: 'btn-inspect', label: 'View Execution Stream', action: '/developer' },
          { id: 'btn-tasks', label: 'Open Tasks Center', action: '/tasks' },
        ],
      };

      set((s) => ({ chatMessages: [...s.chatMessages, vaniMsg] }));

      if (isVoice) {
        speechService.speak(aiResponse.speechText);
      }
    },

    markNotificationAsRead: (id) => {
      set((state) => ({
        notifications: state.notifications.map((n) => (n.id === id ? { ...n, read: true } : n)),
      }));
    },

    clearAllNotifications: () => {
      set({ notifications: [] });
    },
  };
});
