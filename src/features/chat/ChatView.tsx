import React, { useState, useRef, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { motion } from 'framer-motion';
import {
  Send,
  Mic,
  MicOff,
  Paperclip,
  Languages,
  Code2,
  Terminal,
  Bot,
  ExternalLink,
  Sparkles,
  RotateCcw,
  Check,
  Copy,
  Folder,
  Layers,
  StopCircle,
} from 'lucide-react';
import { useVaniStore } from '@/stores/useVaniStore';
import { useVoiceStore } from '@/stores/useVoiceStore';
import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { AudioSpectrumBar } from '@/components/voice/AudioSpectrumBar';
import { slideUpFade } from '@/design-system/motion';

export const ChatView: React.FC = () => {
  const navigate = useNavigate();
  const chatMessages = useVaniStore((s) => s.chatMessages);
  const sendChatMessage = useVaniStore((s) => s.sendChatMessage);
  const voiceState = useVoiceStore((s) => s.state);
  const isMicActive = useVoiceStore((s) => s.isMicActive);
  const setMicActive = useVoiceStore((s) => s.setMicActive);
  const triggerVoicePrompt = useVoiceStore((s) => s.triggerVoicePrompt);
  const interrupt = useVoiceStore((s) => s.interrupt);

  const [input, setInput] = useState('');
  const [copiedId, setCopiedId] = useState<string | null>(null);
  const messagesEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [chatMessages]);

  const handleSend = (e?: React.FormEvent) => {
    if (e) e.preventDefault();
    if (!input.trim()) return;
    sendChatMessage(input.trim(), false);
    setInput('');
  };

  const handleCopyCode = (code: string, id: string) => {
    navigator.clipboard.writeText(code);
    setCopiedId(id);
    setTimeout(() => setCopiedId(null), 2000);
  };

  const samplePrompts = [
    'mera react wala project run karke auth check karde',
    'Write a C++20 thread-safe task queue',
    'Research multi-agent orchestration frameworks',
    'Capture screenshot and analyze active UI errors',
  ];

  return (
    <motion.div
      variants={slideUpFade}
      initial="initial"
      animate="animate"
      className="flex flex-col h-[calc(100vh-8.5rem)] max-w-5xl mx-auto"
    >
      {/* Top Chat Header */}
      <div className="flex items-center justify-between pb-4 border-b border-white/[0.08]">
        <div className="flex items-center gap-3">
          <div className="w-9 h-9 rounded-xl bg-purple-600/20 border border-purple-500/30 flex items-center justify-center">
            <Sparkles className="w-5 h-5 text-purple-400" />
          </div>
          <div>
            <h2 className="text-sm md:text-base font-semibold text-white">VANI Multimodal Canvas</h2>
            <p className="text-xs text-slate-400">Deterministic tools, autonomous coding & voice pipeline</p>
          </div>
        </div>

        {/* Live Audio Status */}
        <div className="flex items-center gap-2">
          {voiceState !== 'idle' && (
            <div className="flex items-center gap-2 px-3 py-1 rounded-full bg-purple-950/40 border border-purple-500/30">
              <AudioSpectrumBar isActive={true} color="purple" barCount={8} />
              <span className="text-[11px] font-mono text-purple-300 capitalize">{voiceState}</span>
            </div>
          )}
          <Badge variant="cyan" size="xs">
            Hinglish Enabled
          </Badge>
        </div>
      </div>

      {/* Message Stream */}
      <div className="flex-1 overflow-y-auto py-6 space-y-6 pr-2">
        {chatMessages.map((msg) => (
          <div
            key={msg.id}
            className={`flex flex-col ${msg.sender === 'user' ? 'items-end' : 'items-start'}`}
          >
            <div className="flex items-center gap-2 mb-1 px-1">
              <span className="text-xs font-semibold text-slate-400">
                {msg.sender === 'user' ? 'You' : 'VANI Core'}
              </span>
              <span className="text-[10px] text-slate-400 font-mono">{msg.timestamp}</span>
              {msg.language && (
                <span className="text-[10px] px-1.5 py-0.2 rounded bg-white/[0.06] text-purple-300 font-mono">
                  {msg.language}
                </span>
              )}
            </div>

            {/* Message Bubble */}
            <div
              className={`max-w-2xl rounded-2xl p-4 md:p-5 space-y-3.5 ${
                msg.sender === 'user'
                  ? 'bg-gradient-to-br from-purple-700/80 to-indigo-700/80 text-white shadow-lg shadow-purple-950/30 border border-purple-400/30 rounded-tr-sm'
                  : 'vani-glass-card text-slate-100 border border-white/10 shadow-xl rounded-tl-sm'
              }`}
            >
              {/* Natural Text / Audio transcript */}
              <p className="text-xs md:text-sm leading-relaxed whitespace-pre-wrap">{msg.text}</p>

              {/* Hinglish Interpretation Breakdown (if user used colloquial Hindi/Hinglish) */}
              {msg.understoodAs && (
                <div className="p-2.5 rounded-xl bg-black/30 border border-cyan-500/20 text-xs text-cyan-200 space-y-1">
                  <div className="flex items-center gap-1.5 text-[10px] uppercase font-mono text-cyan-400 font-bold">
                    <Languages className="w-3 h-3" />
                    <span>Normalized Intent</span>
                  </div>
                  <p className="italic text-slate-300">"{msg.understoodAs}"</p>
                </div>
              )}

              {/* Embedded Live Agent Progress Card */}
              {msg.agentCard && (
                <div
                  onClick={() => navigate(`/tasks/${msg.agentCard?.taskId}`)}
                  className="p-3 rounded-xl bg-black/40 border border-purple-500/30 hover:border-purple-500/60 transition-all cursor-pointer space-y-2 group"
                >
                  <div className="flex items-center justify-between">
                    <div className="flex items-center gap-2">
                      <div className="p-1 rounded bg-purple-500/20 text-purple-300">
                        <Bot className="w-4 h-4" />
                      </div>
                      <div>
                        <h4 className="text-xs font-bold text-white group-hover:text-purple-300 transition-colors">
                          {msg.agentCard.agentName}
                        </h4>
                        <span className="text-[10px] text-slate-400">{msg.agentCard.role}</span>
                      </div>
                    </div>
                    <Badge variant="primary" size="xs" dot>
                      Running ({msg.agentCard.progress}%)
                    </Badge>
                  </div>

                  <p className="text-xs text-slate-300 truncate">{msg.agentCard.taskTitle}</p>

                  <div className="w-full h-1.5 bg-white/10 rounded-full overflow-hidden">
                    <div
                      className="h-full bg-gradient-to-r from-purple-500 to-cyan-400 rounded-full"
                      style={{ width: `${msg.agentCard.progress}%` }}
                    />
                  </div>
                </div>
              )}

              {/* Embedded Tool Executions */}
              {msg.toolsUsed && msg.toolsUsed.length > 0 && (
                <div className="pt-2 border-t border-white/10 flex items-center gap-2 flex-wrap text-[11px] text-slate-300">
                  <span className="text-slate-400 font-mono text-[10px]">Tools Dispatched:</span>
                  {msg.toolsUsed.map((tool, i) => (
                    <span
                      key={i}
                      className="px-2 py-0.5 rounded-md bg-white/[0.06] text-purple-300 border border-purple-500/20 font-mono text-[10px]"
                    >
                      ✓ {tool}
                    </span>
                  ))}
                </div>
              )}

              {/* Code Snippet Box */}
              {msg.codeBlock && (
                <div className="rounded-xl overflow-hidden border border-white/10 bg-[#090d16] font-mono text-xs">
                  <div className="flex items-center justify-between px-3 py-2 bg-white/[0.04] border-b border-white/10 text-slate-400">
                    <div className="flex items-center gap-2">
                      <Code2 className="w-3.5 h-3.5 text-purple-400" />
                      <span className="text-[11px]">{msg.codeBlock.filename || msg.codeBlock.language}</span>
                    </div>
                    <button
                      onClick={() => handleCopyCode(msg.codeBlock!.code, msg.id)}
                      className="flex items-center gap-1 text-[10px] text-slate-400 hover:text-white transition-colors"
                    >
                      {copiedId === msg.id ? (
                        <>
                          <Check className="w-3 h-3 text-emerald-400" /> Copied
                        </>
                      ) : (
                        <>
                          <Copy className="w-3 h-3" /> Copy
                        </>
                      )}
                    </button>
                  </div>
                  <pre className="p-3.5 overflow-x-auto text-purple-200 leading-relaxed">
                    <code>{msg.codeBlock.code}</code>
                  </pre>
                </div>
              )}

              {/* Action Buttons */}
              {msg.actionButtons && msg.actionButtons.length > 0 && (
                <div className="pt-2 flex items-center gap-2 flex-wrap">
                  {msg.actionButtons.map((btn) => (
                    <Button
                      key={btn.id}
                      variant="secondary"
                      size="xs"
                      onClick={() => {
                        if (btn.action.startsWith('/')) {
                          navigate(btn.action);
                        }
                      }}
                      rightIcon={<ExternalLink className="w-3 h-3" />}
                    >
                      {btn.label}
                    </Button>
                  ))}
                </div>
              )}
            </div>
          </div>
        ))}
        <div ref={messagesEndRef} />
      </div>

      {/* Suggested Quick Prompts */}
      <div className="py-2 flex items-center gap-2 overflow-x-auto no-scrollbar">
        <span className="text-[10px] uppercase font-mono text-slate-400 flex-shrink-0">Suggestions:</span>
        {samplePrompts.map((sp, idx) => (
          <button
            key={idx}
            onClick={() => setInput(sp)}
            className="text-xs px-3 py-1 rounded-lg bg-white/[0.03] hover:bg-white/[0.08] border border-white/5 text-slate-300 hover:text-white transition-colors whitespace-nowrap"
          >
            {sp}
          </button>
        ))}
      </div>

      {/* Input Bar */}
      <form
        onSubmit={handleSend}
        className="p-2.5 rounded-2xl bg-[#090d16] border border-white/10 shadow-2xl flex items-center gap-2"
      >
        <button
          type="button"
          onClick={() => {
            if (voiceState === 'speaking' || voiceState === 'executing') {
              interrupt();
            } else {
              setMicActive(!isMicActive);
              if (!isMicActive) {
                triggerVoicePrompt(input || 'mera react wala project run karke check karde');
              }
            }
          }}
          className={`p-2.5 rounded-xl transition-all ${
            isMicActive || voiceState !== 'idle'
              ? 'bg-purple-600 text-white shadow-[0_0_15px_#8b5cf6]'
              : 'text-slate-400 hover:text-white hover:bg-white/[0.06]'
          }`}
          title={isMicActive ? 'Mute Mic' : 'Voice Input'}
        >
          {voiceState === 'speaking' ? (
            <StopCircle className="w-4 h-4 text-white" />
          ) : (
            <Mic className="w-4 h-4" />
          )}
        </button>

        <input
          type="text"
          value={input}
          onChange={(e) => setInput(e.target.value)}
          placeholder="Ask VANI or give a voice/terminal instruction (e.g. 'mera auth wala bug solve karde')..."
          className="flex-1 bg-transparent text-sm text-white placeholder:text-slate-400 outline-none font-medium px-2"
        />

        <Button
          type="submit"
          variant="primary"
          size="sm"
          disabled={!input.trim()}
          rightIcon={<Send className="w-3.5 h-3.5" />}
        >
          Send
        </Button>
      </form>
    </motion.div>
  );
};
