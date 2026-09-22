/**
 * VANI Mark 2 — Advanced Conversational AI Engine
 * Real-time LLM-Powered Dialogue (Local Ollama / Qwen / Llama 3.2 + Neural Synthesis)
 */

export interface ConversationTurn {
  role: 'user' | 'assistant' | 'system';
  content: string;
  timestamp: number;
}

export interface ConversationResponse {
  replyText: string;
  speechText: string;
  understoodIntent?: string;
  dispatchedAction?: string;
  agentCard?: {
    agentName: string;
    role: string;
    taskId: string;
    taskTitle: string;
    progress: number;
  };
  toolsUsed?: string[];
  suggestedFollowUps?: string[];
}

class ConversationEngine {
  private history: ConversationTurn[] = [];
  private preferredModel: string = 'qwen2.5:3b'; // High speed + great Hinglish

  constructor() {
    this.history.push({
      role: 'assistant',
      content: 'Namaste! VANI Mark 2 voice and intelligence core is active and ready.',
      timestamp: Date.now(),
    });
  }

  getHistory(): ConversationTurn[] {
    return this.history;
  }

  clearHistory() {
    this.history = [];
  }

  setModel(modelName: string) {
    this.preferredModel = modelName;
  }

  // Query local Ollama instance for real, intelligent, dynamic conversation
  private async queryLocalLLM(
    userInput: string,
    systemContext?: {
      activeTasksCount: number;
      cpuUsage: number;
      memoryUsage: number;
      isOffline: boolean;
      activeModel: string;
    }
  ): Promise<string | null> {
    try {
      const systemPrompt = `You are VANI (Voice & Autonomous Neural Intelligence), an exceptionally smart, warm, witty, and helpful AI assistant running locally on the user's PC.
- You speak fluent natural Hinglish and English (just like a modern smart Indian tech friend).
- When spoken to in Hinglish/Hindi, reply in conversational Hinglish. When spoken to in English, reply in English.
- Be concise (2-4 punchy sentences) so it sounds great when spoken aloud by TTS.
- You can coordinate autonomous agents (Odysseus for coding, Hermes for research).
- Live System Context: CPU at ${systemContext?.cpuUsage || 18}%, RAM at ${systemContext?.memoryUsage || 42}%, Active Tasks: ${systemContext?.activeTasksCount || 2}.
- Never output markdown headers, raw tables, or XML tags. Speak directly and naturally.`;

      // Build context window (last 6 turns)
      const recentHistory = this.history.slice(-6).map((turn) => ({
        role: turn.role,
        content: turn.content,
      }));

      const messages = [
        { role: 'system', content: systemPrompt },
        ...recentHistory,
        { role: 'user', content: userInput },
      ];

      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 6000); // 6s max timeout for voice speed

      // Try local Ollama or Gateway proxy
      const endpoints = [
        'http://localhost:3002/api/chat',
        'http://localhost:11434/api/chat'
      ];
      const modelsToTry = [this.preferredModel, 'qwen2.5:3b', 'llama3.2:1b', 'llama3.1:8b'];

      for (const endpoint of endpoints) {
        for (const model of modelsToTry) {
          try {
            const response = await fetch(endpoint, {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              signal: controller.signal,
              body: JSON.stringify({
                model,
                messages,
                stream: false,
                options: {
                  temperature: 0.7,
                  top_p: 0.9,
                  num_predict: 120, // Keep short for fast voice response
                },
              }),
            });

            if (response.ok) {
              clearTimeout(timeoutId);
              const data = await response.json();
              let rawContent = data.message?.content || '';

              // Clean any <think> tags or reasoning artifacts from models like deepseek-r1
              rawContent = rawContent.replace(/<think>[\s\S]*?<\/think>/gi, '').trim();

              if (rawContent) {
                return rawContent;
              }
            }
          } catch (e) {
            // Try next
          }
        }
      }

      clearTimeout(timeoutId);
      return null;
    } catch (err) {
      console.warn('[ConversationEngine] Ollama query exception:', err);
      return null;
    }
  }

  // Generate intelligent context-aware response
  async generateResponse(
    userInput: string,
    systemContext?: {
      activeTasksCount: number;
      cpuUsage: number;
      memoryUsage: number;
      isOffline: boolean;
      activeModel: string;
    }
  ): Promise<ConversationResponse> {
    const text = userInput.trim();
    const lower = text.toLowerCase();

    // Context helpers
    const cpu = systemContext?.cpuUsage || 18;
    const mem = systemContext?.memoryUsage || 42;
    const tasks = systemContext?.activeTasksCount || 2;

    // 1. First, attempt Real Dynamic Local LLM inference (Ollama)
    const llmOutput = await this.queryLocalLLM(text, systemContext);

    let reply = '';
    let speech = '';
    let intent: string | undefined = undefined;
    let action: string | undefined = undefined;
    let agentCard: ConversationResponse['agentCard'] = undefined;
    let followUps: string[] = [];

    if (llmOutput && llmOutput.length > 5) {
      reply = llmOutput;
      speech = llmOutput
        .replace(/[*#_`~[\]()]/g, '')
        .replace(/\n+/g, ' ')
        .trim();

      // Derive intent metadata from user utterance
      if (lower.includes('project') || lower.includes('react') || lower.includes('code') || lower.includes('build')) {
        intent = 'Coding Task Execution';
        action = 'Dispatched to Agent Odysseus';
        agentCard = {
          agentName: 'Odysseus',
          role: 'Full-Stack C++ & TypeScript Lead',
          taskId: `task-${Date.now()}`,
          taskTitle: text,
          progress: 15,
        };
      } else if (lower.includes('research') || lower.includes('paper') || lower.includes('search')) {
        intent = 'Research & Synthesis';
        action = 'Dispatched to Agent Hermes';
        agentCard = {
          agentName: 'Hermes',
          role: 'Deep Scientific & Web Synthesis',
          taskId: `task-${Date.now()}`,
          taskTitle: text,
          progress: 10,
        };
      }
    } else {
      // 2. Intelligent Dynamic Semantic Fallback if Ollama is paused
      const isHinglish =
        lower.includes('hai') ||
        lower.includes('kya') ||
        lower.includes('kaise') ||
        lower.includes('batao') ||
        lower.includes('karde') ||
        lower.includes('mera') ||
        lower.includes('karo');

      if (lower.includes('react') || lower.includes('code') || lower.includes('build') || lower.includes('bug')) {
        reply = isHinglish
          ? `Bilkul! Main aapka project compile aur run karne ke liye Agent Odysseus ko assign kar rahi hoon. AST checks aur tests run ho rahe hain.`
          : `Got it! I am dispatching your coding request to Agent Odysseus. He will verify types, run the build, and execute the tests.`;
        speech = reply;
        intent = 'Execute Coding Workflow';
        action = 'agent.odysseus.spawn';
        agentCard = {
          agentName: 'Odysseus',
          role: 'Full-Stack C++ & TypeScript Lead',
          taskId: `task-${Date.now()}`,
          taskTitle: 'Compile & Run React Workflow',
          progress: 25,
        };
        followUps = ['Show live terminal logs', 'Check test coverage', 'Stop task'];
      } else if (lower.includes('status') || lower.includes('system') || lower.includes('health')) {
        reply = isHinglish
          ? `VANI Mark 2 control plane bilkul healthy hai. CPU ${cpu}% par hai, RAM usage ${mem}%, aur ${tasks} background tasks seamlessly execute ho rahe hain.`
          : `All VANI subsystems are healthy. CPU is at ${cpu}%, memory is ${mem}%, and ${tasks} tasks are running smoothly.`;
        speech = reply;
        intent = 'Inspect Runtime Diagnostics';
        action = 'runtime.diagnostics.inspect';
        followUps = ['Show process tree', 'View event bus load', 'Run full audit'];
      } else if (lower.includes('kaise ho') || lower.includes('how are you') || lower.includes('kya haal')) {
        reply = isHinglish
          ? `Main ekdum first-class hoon! Local neural models ready hain aur saare subagents active hain. Aap bataiye, aaj kis project par kaam karna hai?`
          : `I'm doing fantastic! All local neural engines are primed and ready to go. What shall we build or discuss today?`;
        speech = reply;
        intent = 'Conversational Engagement';
        followUps = ['Run React project', 'Check system status', 'Research AI agents'];
      } else {
        reply = isHinglish
          ? `Main samajh gayi! "${text}" ko lekar main VANI runtime router ke sath coordinate kar rahi hoon. Kuch specific execute karna hai?`
          : `Understood! I'm coordinating "${text}" with the VANI runtime router. What specific outcome would you like?`;
        speech = reply;
        intent = text;
        followUps = ['Explain details', 'Assign to agent', 'Show system metrics'];
      }
    }

    // Update conversation memory
    this.history.push({ role: 'user', content: text, timestamp: Date.now() });
    this.history.push({ role: 'assistant', content: reply, timestamp: Date.now() });

    return {
      replyText: reply,
      speechText: speech,
      understoodIntent: intent,
      dispatchedAction: action,
      agentCard,
      suggestedFollowUps: followUps,
    };
  }
}

export const conversationEngine = new ConversationEngine();
