export type VoiceState =
  | 'idle'
  | 'listening'
  | 'thinking'
  | 'executing'
  | 'speaking'
  | 'waiting_confirmation'
  | 'error'
  | 'offline';

export type TaskCategory =
  | 'coding'
  | 'research'
  | 'automation'
  | 'system'
  | 'perception'
  | 'integration'
  | 'custom';

export type TaskState =
  | 'running'
  | 'queued'
  | 'waiting'
  | 'completed'
  | 'failed'
  | 'cancelled';

export type TaskPriority = 'low' | 'normal' | 'high' | 'critical';

export interface TaskToolUsage {
  name: string;
  icon?: string;
  status: 'pending' | 'active' | 'completed' | 'failed';
  outputSnippet?: string;
}

export interface TaskArtifact {
  id: string;
  name: string;
  type: 'code' | 'file' | 'markdown' | 'image' | 'json';
  content?: string;
  size?: string;
}

export interface TaskTimelineEvent {
  id: string;
  timestamp: string;
  description: string;
  type: 'info' | 'tool' | 'agent' | 'success' | 'warning' | 'error';
}

export interface Task {
  id: string;
  title: string;
  description?: string;
  category: TaskCategory;
  state: TaskState;
  progress?: number;
  startedAt?: string;
  completedAt?: string;
  agentId?: string;
  agentName?: string;
  agentAvatar?: string;
  priority: TaskPriority;
  toolsUsed?: TaskToolUsage[];
  artifacts?: TaskArtifact[];
  timeline?: TaskTimelineEvent[];
  modelUsed?: string;
  logs?: string[];
  result?: string;
}

export type AgentRole =
  | 'Coding Agent'
  | 'Research Agent'
  | 'Automation Agent'
  | 'Perception Agent'
  | 'Security Agent'
  | 'Custom Agent';

export interface AgentCapability {
  id: string;
  name: string;
  enabled: boolean;
}

export interface Agent {
  id: string;
  name: string;
  role: AgentRole;
  avatar: string;
  status: 'idle' | 'running' | 'waiting' | 'disabled';
  health: 'healthy' | 'warning' | 'error';
  currentTaskId?: string;
  currentTaskTitle?: string;
  progress?: number;
  model: string;
  description: string;
  capabilities: AgentCapability[];
  permissions: string[];
  tools: string[];
  memoryAccess: 'full' | 'project-only' | 'read-only' | 'none';
  completedTasksCount: number;
  avgLatencyMs: number;
}

export type RiskLevel = 'low' | 'medium' | 'high' | 'restricted';

export interface Tool {
  id: string;
  name: string;
  description: string;
  icon: string;
  riskLevel: RiskLevel;
  status: 'active' | 'idle' | 'disabled' | 'restricted';
  permissions: string[];
  version: string;
  lastUsed?: string;
  category: 'system' | 'os' | 'developer' | 'browser' | 'media';
  executionCount: number;
}

export interface Model {
  id: string;
  name: string;
  provider: string;
  type: 'local' | 'cloud' | 'remote';
  contextSize: string;
  maxTokens: number;
  activeContextTokens: number;
  capabilities: ('text' | 'vision' | 'tools' | 'coding' | 'voice')[];
  latencyMs: number;
  status: 'active' | 'available' | 'offline' | 'paused';
  priority: number;
  ttftMs: number;
  description: string;
}

export interface MemoryItem {
  id: string;
  title: string;
  content: string;
  category: 'working' | 'long-term' | 'projects' | 'preferences' | 'vocabulary' | 'knowledge';
  source: string;
  confidence: number;
  importance: 'low' | 'medium' | 'high' | 'critical';
  scope: string;
  createdAt: string;
  lastAccessed: string;
  isPinned?: boolean;
  whyReason?: string;
}

export interface Integration {
  id: string;
  name: string;
  category: 'Development' | 'Communication' | 'Productivity' | 'Education' | 'Business' | 'Automation';
  icon: string;
  description: string;
  status: 'connected' | 'disconnected' | 'needs_attention' | 'permission_required' | 'offline';
  account?: string;
  toolsProvided: string[];
  permissions: string[];
  lastSync?: string;
  metrics?: {
    label: string;
    value: string | number;
  }[];
}

export interface ConnectedDevice {
  id: string;
  name: string;
  type: 'computer' | 'phone' | 'buds' | 'watch' | 'wearable';
  status: 'connected' | 'disconnected' | 'pairing';
  batteryPercentage?: number;
  lastSeen: string;
  latencyMs: number;
  hasMicrophone: boolean;
  hasCamera: boolean;
  permissions: string[];
  modelInfo?: string;
}

export interface PermissionRequest {
  id: string;
  timestamp: string;
  title: string;
  description: string;
  sourceAgent: string;
  sourceTool: string;
  riskLevel: RiskLevel;
  status: 'pending' | 'allowed' | 'denied';
  details?: Record<string, any>;
}

export interface SystemMetric {
  cpuUsage: number;
  ramUsedGb: number;
  ramTotalGb: number;
  gpuUsage: number;
  diskUsedGb: number;
  diskTotalGb: number;
  networkLatencyMs: number;
  networkBandwidthMbps: number;
  sttStatus: 'active' | 'standby' | 'error';
  ttsStatus: 'active' | 'standby' | 'error';
  screenPerception: 'active' | 'idle' | 'off';
  cameraPerception: 'active' | 'off';
  micPerception: 'active' | 'muted' | 'off';
  ocrStatus: 'ready' | 'processing' | 'standby';
}

export interface ActivityEvent {
  id: string;
  timestamp: string;
  agent?: string;
  action: string;
  target?: string;
  type: 'voice' | 'agent' | 'task' | 'tool' | 'system' | 'security';
  details?: string;
}

export interface NotificationItem {
  id: string;
  title: string;
  message: string;
  timestamp: string;
  type: 'info' | 'success' | 'warning' | 'error';
  read: boolean;
  actionUrl?: string;
}

export interface ChatMessage {
  id: string;
  sender: 'user' | 'vani';
  timestamp: string;
  text: string;
  rawTranscript?: string;
  understoodAs?: string;
  voiceUsed?: boolean;
  language?: 'English' | 'Hinglish' | 'Hindi';
  agentCard?: {
    agentName: string;
    role: string;
    taskId: string;
    taskTitle: string;
    progress: number;
  };
  toolsUsed?: string[];
  codeBlock?: {
    language: string;
    code: string;
    filename?: string;
  };
  actionButtons?: {
    id: string;
    label: string;
    action: string;
  }[];
  confirmationRequest?: {
    id: string;
    title: string;
    description: string;
    risk: RiskLevel;
  };
}
