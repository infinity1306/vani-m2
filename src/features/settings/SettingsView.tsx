import React, { useState } from 'react';
import { motion } from 'framer-motion';
import {
  Settings,
  Mic,
  Languages,
  Sliders,
  Cpu,
  Brain,
  Shield,
  Smartphone,
  Bell,
  Keyboard,
  Activity,
  Terminal,
  Save,
  Check,
} from 'lucide-react';
import { useSettingsStore } from '@/stores/useSettingsStore';
import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { slideUpFade } from '@/design-system/motion';

export const SettingsView: React.FC = () => {
  const settings = useSettingsStore();
  const updateSettings = useSettingsStore((s) => s.updateSettings);

  const [activeCategory, setActiveCategory] = useState<'voice' | 'models' | 'security' | 'privacy' | 'general'>('voice');
  const [savedToast, setSavedToast] = useState(false);

  const categories = [
    { id: 'voice', label: 'Voice & Acoustic', icon: <Mic className="w-4 h-4" /> },
    { id: 'models', label: 'AI Providers & Local Mesh', icon: <Cpu className="w-4 h-4" /> },
    { id: 'privacy', label: 'Privacy & Screen Masking', icon: <Shield className="w-4 h-4" /> },
    { id: 'security', label: 'Security Guardrails', icon: <Shield className="w-4 h-4" /> },
    { id: 'general', label: 'General & Keyboard', icon: <Keyboard className="w-4 h-4" /> },
  ];

  const handleSave = () => {
    setSavedToast(true);
    setTimeout(() => setSavedToast(false), 2500);
  };

  return (
    <motion.div variants={slideUpFade} initial="initial" animate="animate" className="space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl md:text-2xl font-bold text-white tracking-tight">System Configuration</h1>
          <p className="text-xs md:text-sm text-slate-400">
            Fine-tune VANI voice models, execution sandboxing, local-first priorities, and privacy thresholds.
          </p>
        </div>
        <Button
          variant="primary"
          size="sm"
          leftIcon={savedToast ? <Check className="w-4 h-4" /> : <Save className="w-4 h-4" />}
          onClick={handleSave}
        >
          {savedToast ? 'Settings Applied' : 'Save Changes'}
        </Button>
      </div>

      {/* Main Settings Grid */}
      <div className="grid grid-cols-1 md:grid-cols-4 gap-6">
        {/* Categories Sidebar */}
        <div className="space-y-1">
          {categories.map((cat) => (
            <button
              key={cat.id}
              onClick={() => setActiveCategory(cat.id as any)}
              className={`w-full flex items-center gap-3 px-3.5 py-2.5 rounded-xl text-xs font-medium transition-all text-left ${
                activeCategory === cat.id
                  ? 'bg-purple-600/20 text-white border border-purple-500/40 shadow-sm'
                  : 'text-slate-400 hover:text-slate-200 hover:bg-white/[0.04]'
              }`}
            >
              <span>{cat.icon}</span>
              <span>{cat.label}</span>
            </button>
          ))}
        </div>

        {/* Settings Panel */}
        <div className="md:col-span-3 space-y-6">
          {activeCategory === 'voice' && (
            <Card className="space-y-5">
              <h3 className="text-sm font-bold text-white border-b border-white/10 pb-3">
                Voice & Acoustic Interaction
              </h3>

              <div className="space-y-4">
                <div>
                  <label className="text-xs font-mono text-slate-300 block mb-1">TTS Neural Voice</label>
                  <select
                    value={settings.ttsVoice}
                    onChange={(e) => updateSettings({ ttsVoice: e.target.value })}
                    className="w-full px-3 py-2 rounded-xl bg-[#090d16] border border-white/10 text-xs text-white outline-none"
                  >
                    <option value="VANI Neural Expressive (Alto)">VANI Neural Expressive (Alto)</option>
                    <option value="VANI Fast Technical (Bilingual)">VANI Fast Technical (Bilingual)</option>
                    <option value="VANI Calm Editorial">VANI Calm Editorial</option>
                  </select>
                </div>

                <div>
                  <div className="flex justify-between text-xs font-mono text-slate-300 mb-1">
                    <span>Speech Playback Speed ({settings.speechSpeed}x)</span>
                  </div>
                  <input
                    type="range"
                    min="0.8"
                    max="1.5"
                    step="0.05"
                    value={settings.speechSpeed}
                    onChange={(e) => updateSettings({ speechSpeed: parseFloat(e.target.value) })}
                    className="w-full accent-purple-500 cursor-pointer"
                  />
                </div>

                <div className="flex items-center justify-between pt-2 border-t border-white/5">
                  <div>
                    <h4 className="text-xs font-semibold text-white">Auto-speak Agent Summaries</h4>
                    <p className="text-[11px] text-slate-400">Read finished task briefs aloud through spatial audio</p>
                  </div>
                  <input
                    type="checkbox"
                    checked={settings.autoSpeak}
                    onChange={(e) => updateSettings({ autoSpeak: e.target.checked })}
                    className="w-4 h-4 accent-purple-500 cursor-pointer"
                  />
                </div>
              </div>
            </Card>
          )}

          {activeCategory === 'models' && (
            <Card className="space-y-5">
              <h3 className="text-sm font-bold text-white border-b border-white/10 pb-3">
                AI Engines & Local Mesh
              </h3>

              <div className="space-y-4">
                <div className="flex items-center justify-between">
                  <div>
                    <h4 className="text-xs font-semibold text-white">Prioritize Local Hardware (Ollama / Vulkan)</h4>
                    <p className="text-[11px] text-slate-400">Keep all code generation & deterministic tools local</p>
                  </div>
                  <input
                    type="checkbox"
                    checked={settings.localModelPriority}
                    onChange={(e) => updateSettings({ localModelPriority: e.target.checked })}
                    className="w-4 h-4 accent-purple-500 cursor-pointer"
                  />
                </div>

                <div className="flex items-center justify-between pt-2 border-t border-white/5">
                  <div>
                    <h4 className="text-xs font-semibold text-white">Allow Cloud Fallbacks for Massive Research</h4>
                    <p className="text-[11px] text-slate-400">Route 100k+ token research queries to Gemini 1.5 Pro</p>
                  </div>
                  <input
                    type="checkbox"
                    checked={settings.cloudFallbacksAllowed}
                    onChange={(e) => updateSettings({ cloudFallbacksAllowed: e.target.checked })}
                    className="w-4 h-4 accent-purple-500 cursor-pointer"
                  />
                </div>
              </div>
            </Card>
          )}

          {activeCategory === 'privacy' && (
            <Card className="space-y-5">
              <h3 className="text-sm font-bold text-white border-b border-white/10 pb-3">
                Screen Perception & Privacy Masking
              </h3>

              <div className="space-y-4">
                <div className="flex items-center justify-between">
                  <div>
                    <h4 className="text-xs font-semibold text-white">Neural Screen Privacy Masking</h4>
                    <p className="text-[11px] text-slate-400">Automatically blur passwords and banking tokens before OCR</p>
                  </div>
                  <input
                    type="checkbox"
                    checked={settings.privacyMaskScreenshots}
                    onChange={(e) => updateSettings({ privacyMaskScreenshots: e.target.checked })}
                    className="w-4 h-4 accent-purple-500 cursor-pointer"
                  />
                </div>

                <div className="flex items-center justify-between pt-2 border-t border-white/5">
                  <div>
                    <h4 className="text-xs font-semibold text-white">Anonymous Telemetry</h4>
                    <p className="text-[11px] text-slate-400">Send anonymized crash reports to local development log</p>
                  </div>
                  <input
                    type="checkbox"
                    checked={settings.telemetryEnabled}
                    onChange={(e) => updateSettings({ telemetryEnabled: e.target.checked })}
                    className="w-4 h-4 accent-purple-500 cursor-pointer"
                  />
                </div>
              </div>
            </Card>
          )}

          {activeCategory === 'security' && (
            <Card className="space-y-5">
              <h3 className="text-sm font-bold text-white border-b border-white/10 pb-3">
                Sandboxing & Privilege Bounds
              </h3>

              <div className="space-y-4">
                <div className="flex items-center justify-between">
                  <div>
                    <h4 className="text-xs font-semibold text-white">Strict Container Isolation</h4>
                    <p className="text-[11px] text-slate-400">Require manual approval for commands executing outside workspace</p>
                  </div>
                  <input
                    type="checkbox"
                    checked={settings.strictSandboxing}
                    onChange={(e) => updateSettings({ strictSandboxing: e.target.checked })}
                    className="w-4 h-4 accent-purple-500 cursor-pointer"
                  />
                </div>
              </div>
            </Card>
          )}

          {activeCategory === 'general' && (
            <Card className="space-y-5">
              <h3 className="text-sm font-bold text-white border-b border-white/10 pb-3">
                Keyboard Shortcuts & General
              </h3>

              <div className="space-y-3 text-xs font-mono">
                <div className="flex justify-between items-center p-2.5 rounded-lg bg-black/30 border border-white/5">
                  <span className="text-slate-300 font-sans">Command Palette</span>
                  <kbd className="px-2 py-1 rounded bg-white/10 text-purple-300">⌘K / Ctrl+K</kbd>
                </div>
                <div className="flex justify-between items-center p-2.5 rounded-lg bg-black/30 border border-white/5">
                  <span className="text-slate-300 font-sans">Push to Talk</span>
                  <kbd className="px-2 py-1 rounded bg-white/10 text-cyan-300">Space (Hold)</kbd>
                </div>
                <div className="flex justify-between items-center p-2.5 rounded-lg bg-black/30 border border-white/5">
                  <span className="text-slate-300 font-sans">Interrupt Voice / Task</span>
                  <kbd className="px-2 py-1 rounded bg-white/10 text-rose-300">Escape</kbd>
                </div>
              </div>
            </Card>
          )}
        </div>
      </div>
    </motion.div>
  );
};
