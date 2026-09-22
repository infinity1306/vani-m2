import { create } from 'zustand';

interface SettingsState {
  userName: string;
  userRole: string;
  theme: 'dark' | 'midnight' | 'cyber';
  ttsVoice: string;
  speechSpeed: number;
  autoSpeak: boolean;
  pushToTalkKey: string;
  commandPaletteKey: string;
  haptics: boolean;
  privacyMaskScreenshots: boolean;
  cloudFallbacksAllowed: boolean;
  localModelPriority: boolean;
  strictSandboxing: boolean;
  telemetryEnabled: boolean;

  // Actions
  updateSettings: (updates: Partial<SettingsState>) => void;
}

export const useSettingsStore = create<SettingsState>((set) => ({
  userName: 'Anant Sharma',
  userRole: 'Staff AI Systems Engineer',
  theme: 'dark',
  ttsVoice: 'VANI Neural Expressive (Alto)',
  speechSpeed: 1.05,
  autoSpeak: true,
  pushToTalkKey: 'Space (Hold)',
  commandPaletteKey: '⌘K / Ctrl+K',
  haptics: true,
  privacyMaskScreenshots: true,
  cloudFallbacksAllowed: true,
  localModelPriority: true,
  strictSandboxing: true,
  telemetryEnabled: false,

  updateSettings: (updates) => set((state) => ({ ...state, ...updates })),
}));
