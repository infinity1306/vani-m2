import React, { useState } from 'react';
import { motion } from 'framer-motion';
import {
  Brain,
  Search,
  Pin,
  Trash2,
  HelpCircle,
  Sparkles,
  Info,
  Plus,
  ShieldCheck,
  Tag,
} from 'lucide-react';
import { useVaniStore } from '@/stores/useVaniStore';
import { MemoryItem } from '@/types';
import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { Modal } from '@/components/ui/Modal';
import { slideUpFade } from '@/design-system/motion';

export const MemoryView: React.FC = () => {
  const memory = useVaniStore((s) => s.memory);
  const forgetMemory = useVaniStore((s) => s.forgetMemory);
  const togglePinMemory = useVaniStore((s) => s.togglePinMemory);
  const addMemory = useVaniStore((s) => s.addMemory);

  const [activeCategory, setActiveCategory] = useState<string>('all');
  const [searchQuery, setSearchQuery] = useState('');
  const [whyModalItem, setWhyModalItem] = useState<MemoryItem | null>(null);
  const [isAddModalOpen, setIsAddModalOpen] = useState(false);
  const [newTitle, setNewTitle] = useState('');
  const [newContent, setNewContent] = useState('');
  const [newCategory, setNewCategory] = useState<MemoryItem['category']>('preferences');

  const categories = [
    { id: 'all', label: 'All Memory' },
    { id: 'working', label: 'Working' },
    { id: 'long-term', label: 'Long-term' },
    { id: 'projects', label: 'Projects' },
    { id: 'preferences', label: 'Preferences' },
    { id: 'vocabulary', label: 'Vocabulary' },
    { id: 'knowledge', label: 'Knowledge' },
  ];

  const filteredMemory = memory.filter((item) => {
    const matchesCat = activeCategory === 'all' || item.category === activeCategory;
    const matchesSearch =
      item.title.toLowerCase().includes(searchQuery.toLowerCase()) ||
      item.content.toLowerCase().includes(searchQuery.toLowerCase());
    return matchesCat && matchesSearch;
  });

  const handleAdd = (e: React.FormEvent) => {
    e.preventDefault();
    if (!newTitle.trim() || !newContent.trim()) return;
    addMemory({
      title: newTitle,
      content: newContent,
      category: newCategory,
      source: 'User Explicit Configuration',
      confidence: 1.0,
      importance: 'high',
      scope: 'global',
      isPinned: true,
      whyReason: 'Manually entered by user in Memory Control Layer.',
    });
    setNewTitle('');
    setNewContent('');
    setIsAddModalOpen(false);
  };

  return (
    <motion.div variants={slideUpFade} initial="initial" animate="animate" className="space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl md:text-2xl font-bold text-white tracking-tight">System Memory & Context</h1>
          <p className="text-xs md:text-sm text-slate-400">
            Transparent, controllable intelligence layer powering agent reasoning and personalized workflows.
          </p>
        </div>
        <Button
          variant="primary"
          size="sm"
          leftIcon={<Plus className="w-4 h-4" />}
          onClick={() => setIsAddModalOpen(true)}
        >
          Add Explicit Memory
        </Button>
      </div>

      {/* Categories & Search */}
      <div className="flex flex-col md:flex-row items-stretch md:items-center justify-between gap-3">
        <div className="flex items-center gap-1.5 p-1 rounded-xl bg-white/[0.03] border border-white/10 overflow-x-auto">
          {categories.map((cat) => (
            <button
              key={cat.id}
              onClick={() => setActiveCategory(cat.id)}
              className={`px-3 py-1.5 rounded-lg text-xs font-medium transition-all whitespace-nowrap ${
                activeCategory === cat.id
                  ? 'bg-purple-600 text-white shadow-sm'
                  : 'text-slate-400 hover:text-white hover:bg-white/[0.04]'
              }`}
            >
              {cat.label}
            </button>
          ))}
        </div>

        <div className="relative">
          <Search className="w-3.5 h-3.5 text-slate-400 absolute left-3 top-1/2 -translate-y-1/2" />
          <input
            type="text"
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            placeholder="Search memories, vocabulary, facts..."
            className="w-full md:w-64 pl-9 pr-3 py-1.5 rounded-xl bg-white/[0.03] border border-white/10 text-xs text-white placeholder:text-slate-400 outline-none focus:border-purple-500/50"
          />
        </div>
      </div>

      {/* Memory Cards Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
        {filteredMemory.map((item) => (
          <Card key={item.id} className="flex flex-col justify-between space-y-4">
            <div className="space-y-2.5">
              <div className="flex items-start justify-between gap-2">
                <div className="flex items-center gap-2">
                  <Badge variant="primary" size="xs">
                    {item.category}
                  </Badge>
                  {item.isPinned && (
                    <span className="p-1 rounded bg-purple-500/20 text-purple-300">
                      <Pin className="w-3 h-3 fill-current" />
                    </span>
                  )}
                </div>
                <div className="flex items-center gap-1">
                  <button
                    onClick={() => setWhyModalItem(item)}
                    title="Why does VANI remember this?"
                    className="p-1 text-slate-400 hover:text-cyan-300 transition-colors"
                  >
                    <HelpCircle className="w-3.5 h-3.5" />
                  </button>
                  <button
                    onClick={() => togglePinMemory(item.id)}
                    title={item.isPinned ? 'Unpin' : 'Pin'}
                    className="p-1 text-slate-400 hover:text-purple-300 transition-colors"
                  >
                    <Pin className="w-3.5 h-3.5" />
                  </button>
                  <button
                    onClick={() => forgetMemory(item.id)}
                    title="Forget permanently"
                    className="p-1 text-slate-400 hover:text-rose-400 transition-colors"
                  >
                    <Trash2 className="w-3.5 h-3.5" />
                  </button>
                </div>
              </div>

              <h3 className="text-sm font-semibold text-white leading-snug">{item.title}</h3>
              <p className="text-xs text-slate-300 leading-relaxed">{item.content}</p>
            </div>

            <div className="pt-2 border-t border-white/5 space-y-1.5 text-[11px] font-mono text-slate-400">
              <div className="flex items-center justify-between">
                <span>Confidence</span>
                <span className="text-emerald-400">{Math.round(item.confidence * 100)}%</span>
              </div>
              <div className="flex items-center justify-between text-slate-400 truncate">
                <span>Scope: {item.scope}</span>
                <span>{item.createdAt}</span>
              </div>
            </div>
          </Card>
        ))}
      </div>

      {/* "Why does VANI remember this?" Modal */}
      <Modal
        isOpen={!!whyModalItem}
        onClose={() => setWhyModalItem(null)}
        title="Memory Transparency Audit"
        subtitle="Explainable Context Origin"
      >
        {whyModalItem && (
          <div className="space-y-4">
            <div className="p-3.5 rounded-xl bg-purple-950/20 border border-purple-500/30 text-xs space-y-1">
              <span className="font-semibold text-white">{whyModalItem.title}</span>
              <p className="text-slate-300">{whyModalItem.content}</p>
            </div>

            <div className="space-y-2 text-xs">
              <h4 className="font-mono text-cyan-400 uppercase font-semibold text-[11px]">Origin & Rationale</h4>
              <p className="text-slate-200 bg-white/[0.03] p-3 rounded-lg border border-white/5 leading-relaxed">
                {whyModalItem.whyReason || 'Extracted automatically from active IDE and voice session context.'}
              </p>
            </div>

            <div className="grid grid-cols-2 gap-2 text-xs font-mono">
              <div className="p-2.5 rounded-lg bg-black/30 border border-white/5">
                <span className="text-slate-400 text-[10px] block">Source Engine</span>
                <span className="text-white font-medium">{whyModalItem.source}</span>
              </div>
              <div className="p-2.5 rounded-lg bg-black/30 border border-white/5">
                <span className="text-slate-400 text-[10px] block">Importance</span>
                <span className="text-white font-medium capitalize">{whyModalItem.importance}</span>
              </div>
            </div>

            <div className="flex justify-between items-center pt-3 border-t border-white/10">
              <Button
                variant="danger"
                size="xs"
                leftIcon={<Trash2 className="w-3.5 h-3.5" />}
                onClick={() => {
                  forgetMemory(whyModalItem.id);
                  setWhyModalItem(null);
                }}
              >
                Forget this memory
              </Button>
              <Button variant="secondary" size="xs" onClick={() => setWhyModalItem(null)}>
                Close
              </Button>
            </div>
          </div>
        )}
      </Modal>

      {/* Add Memory Modal */}
      <Modal
        isOpen={isAddModalOpen}
        onClose={() => setIsAddModalOpen(false)}
        title="Add Explicit Memory"
        subtitle="Store persistent preference, knowledge, or project context"
      >
        <form onSubmit={handleAdd} className="space-y-4">
          <div>
            <label className="text-xs font-mono text-slate-400 block mb-1">Title</label>
            <input
              type="text"
              autoFocus
              value={newTitle}
              onChange={(e) => setNewTitle(e.target.value)}
              placeholder="e.g. Always use C++20 for performance tasks"
              className="w-full px-3 py-2 rounded-xl bg-white/[0.04] border border-white/10 text-sm text-white placeholder:text-slate-500 outline-none focus:border-purple-500/50"
            />
          </div>

          <div>
            <label className="text-xs font-mono text-slate-400 block mb-1">Category</label>
            <select
              value={newCategory}
              onChange={(e) => setNewCategory(e.target.value as any)}
              className="w-full px-3 py-2 rounded-xl bg-[#0b0f19] border border-white/10 text-sm text-white outline-none"
            >
              <option value="preferences">Preferences</option>
              <option value="projects">Projects</option>
              <option value="vocabulary">Vocabulary</option>
              <option value="knowledge">Knowledge</option>
              <option value="long-term">Long-term</option>
            </select>
          </div>

          <div>
            <label className="text-xs font-mono text-slate-400 block mb-1">Memory Content</label>
            <textarea
              rows={3}
              value={newContent}
              onChange={(e) => setNewContent(e.target.value)}
              placeholder="Detailed rule, architectural standard, or preference..."
              className="w-full px-3 py-2 rounded-xl bg-white/[0.04] border border-white/10 text-sm text-white placeholder:text-slate-500 outline-none focus:border-purple-500/50"
            />
          </div>

          <div className="flex justify-end gap-2 pt-3 border-t border-white/10">
            <Button variant="ghost" size="sm" type="button" onClick={() => setIsAddModalOpen(false)}>
              Cancel
            </Button>
            <Button variant="primary" size="sm" type="submit" disabled={!newTitle.trim() || !newContent.trim()}>
              Save Memory
            </Button>
          </div>
        </form>
      </Modal>
    </motion.div>
  );
};
