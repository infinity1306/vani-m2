import { eventBus } from './eventBus';
import { Task, VoiceState } from '@/types';

export class VaniRuntimeBridge {
  private isOfflineMode: boolean = false;

  setOfflineMode(offline: boolean) {
    this.isOfflineMode = offline;
    eventBus.emit('runtime:connectivity:changed', { isOffline: offline });
  }

  getIsOffline(): boolean {
    return this.isOfflineMode;
  }

  // Voice execution bridge
  processVoiceInput(rawTranscript: string): Promise<{
    recognized: string;
    normalized: string;
    dispatchedAction: string;
    agentAssigned?: string;
  }> {
    return new Promise((resolve) => {
      // Simulate real-time acoustic pipeline
      eventBus.emit('voice:state:changed', 'thinking' as VoiceState);

      setTimeout(() => {
        let normalized = rawTranscript;
        let dispatchedAction = 'Direct system execution';
        let agentAssigned: string | undefined = 'Odysseus';

        const lower = rawTranscript.toLowerCase();
        if (lower.includes('react') || lower.includes('auth') || lower.includes('code') || lower.includes('app')) {
          normalized = 'Inspect code repository and execute software development task.';
          dispatchedAction = 'Created coding task with Odysseus agent';
          agentAssigned = 'Odysseus';
        } else if (lower.includes('research') || lower.includes('agent') || lower.includes('find') || lower.includes('paper')) {
          normalized = 'Conduct structured research synthesis across scientific & web sources.';
          dispatchedAction = 'Dispatched research task to Hermes';
          agentAssigned = 'Hermes';
        } else if (lower.includes('chrome') || lower.includes('terminal') || lower.includes('screenshot') || lower.includes('open')) {
          normalized = `Execute system tool: ${rawTranscript}`;
          dispatchedAction = 'Invoked OS Window Manager & tool runner';
          agentAssigned = undefined;
        }

        eventBus.emit('voice:state:changed', 'executing' as VoiceState);
        eventBus.emit('activity:created', {
          id: `act-${Date.now()}`,
          timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
          action: `Voice command: "${rawTranscript}"`,
          target: dispatchedAction,
          type: 'voice',
        });

        setTimeout(() => {
          eventBus.emit('voice:state:changed', 'speaking' as VoiceState);
          resolve({
            recognized: rawTranscript,
            normalized,
            dispatchedAction,
            agentAssigned,
          });
        }, 600);
      }, 700);
    });
  }

  // Task simulation engine
  simulateTaskProgress(task: Task, onUpdate: (updated: Task) => void) {
    let currentProgress = task.progress || 0;
    const interval = setInterval(() => {
      currentProgress += Math.floor(Math.random() * 8) + 4;
      if (currentProgress >= 100) {
        currentProgress = 100;
        clearInterval(interval);
        const completedTask: Task = {
          ...task,
          progress: 100,
          state: 'completed',
          completedAt: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
        };
        onUpdate(completedTask);
        eventBus.emit('task:completed', completedTask);
        eventBus.emit('notification:created', {
          id: `notif-${Date.now()}`,
          title: 'Task Completed',
          message: `${task.title} finished successfully.`,
          timestamp: 'Just now',
          type: 'success',
          read: false,
        });
      } else {
        const updated: Task = { ...task, progress: currentProgress };
        onUpdate(updated);
        eventBus.emit('task:updated', updated);
      }
    }, 1200);

    return () => clearInterval(interval);
  }
}

export const vaniRuntime = new VaniRuntimeBridge();
