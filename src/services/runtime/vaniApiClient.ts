import { eventBus } from './eventBus';
import { Task, SystemMetric, VoiceState } from '@/types';

export interface RuntimeStatusResponse {
  state: string;
  version: string;
  contractAbiVersion: string;
  uptimeSeconds: number;
  subsystems: Record<string, { status: string; message: string }>;
  metrics: SystemMetric;
}

export class VaniApiClient {
  private baseUrl: string = 'http://localhost:3002';
  private eventSource: EventSource | null = null;
  private isConnected: boolean = false;
  private reconnectTimer: NodeJS.Timeout | null = null;

  constructor() {
    this.initRealtimeStream();
  }

  // Check connection
  getIsConnected(): boolean {
    return this.isConnected;
  }

  // Initialize Server-Sent Events (SSE) live stream
  initRealtimeStream() {
    if (typeof window === 'undefined') return;

    try {
      if (this.eventSource) {
        this.eventSource.close();
      }

      this.eventSource = new EventSource(`${this.baseUrl}/api/events`);

      this.eventSource.onopen = () => {
        this.isConnected = true;
        eventBus.emit('runtime:connection:changed', { connected: true });
        console.log('[VANI Gateway] Connected to live backend runtime stream.');
      };

      this.eventSource.addEventListener('system:metrics', (e) => {
        try {
          const metrics = JSON.parse((e as MessageEvent).data);
          eventBus.emit('system:metrics:updated', metrics);
        } catch (err) {
          console.error('[VANI Gateway] Error parsing metric event', err);
        }
      });

      this.eventSource.addEventListener('task:created', (e) => {
        try {
          const task = JSON.parse((e as MessageEvent).data);
          eventBus.emit('task:created', task);
        } catch (err) {
          console.error('[VANI Gateway] Error parsing task created event', err);
        }
      });

      this.eventSource.addEventListener('task:progress', (e) => {
        try {
          const task = JSON.parse((e as MessageEvent).data);
          eventBus.emit('task:updated', task);
        } catch (err) {
          console.error('[VANI Gateway] Error parsing task progress event', err);
        }
      });

      this.eventSource.addEventListener('task:completed', (e) => {
        try {
          const task = JSON.parse((e as MessageEvent).data);
          eventBus.emit('task:completed', task);
        } catch (err) {
          console.error('[VANI Gateway] Error parsing task completed event', err);
        }
      });

      this.eventSource.addEventListener('voice:state', (e) => {
        try {
          const { state } = JSON.parse((e as MessageEvent).data);
          eventBus.emit('voice:state:changed', state as VoiceState);
        } catch (err) {
          console.error('[VANI Gateway] Error parsing voice state event', err);
        }
      });

      this.eventSource.addEventListener('activity:created', (e) => {
        try {
          const act = JSON.parse((e as MessageEvent).data);
          eventBus.emit('activity:created', act);
        } catch (err) {
          console.error('[VANI Gateway] Error parsing activity event', err);
        }
      });

      this.eventSource.onerror = () => {
        this.isConnected = false;
        eventBus.emit('runtime:connection:changed', { connected: false });
        if (this.eventSource) {
          this.eventSource.close();
          this.eventSource = null;
        }

        // Retry connection every 5 seconds
        if (!this.reconnectTimer) {
          this.reconnectTimer = setTimeout(() => {
            this.reconnectTimer = null;
            this.initRealtimeStream();
          }, 5000);
        }
      };
    } catch (err) {
      console.warn('[VANI Gateway] Offline or fallback mode active');
    }
  }

  // REST API Methods
  async getStatus(): Promise<RuntimeStatusResponse | null> {
    try {
      const res = await fetch(`${this.baseUrl}/api/status`);
      if (!res.ok) return null;
      return await res.json();
    } catch (err) {
      return null;
    }
  }

  async getCapabilities() {
    try {
      const res = await fetch(`${this.baseUrl}/api/capabilities`);
      if (!res.ok) return [];
      const data = await res.json();
      return data.capabilities || [];
    } catch (err) {
      return [];
    }
  }

  async getTasks(): Promise<Task[]> {
    try {
      const res = await fetch(`${this.baseUrl}/api/tasks`);
      if (!res.ok) return [];
      const data = await res.json();
      return data.tasks || [];
    } catch (err) {
      return [];
    }
  }

  async createTask(spec: Partial<Task>): Promise<Task | null> {
    try {
      const res = await fetch(`${this.baseUrl}/api/tasks`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(spec),
      });
      if (!res.ok) return null;
      const data = await res.json();
      return data.task || null;
    } catch (err) {
      return null;
    }
  }

  async processVoice(transcript: string) {
    try {
      const res = await fetch(`${this.baseUrl}/api/voice/process`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ transcript }),
      });
      if (!res.ok) return null;
      return await res.json();
    } catch (err) {
      return null;
    }
  }
}

export const vaniApiClient = new VaniApiClient();
