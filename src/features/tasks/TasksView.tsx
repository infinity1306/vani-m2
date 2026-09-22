import React, { useState } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import { motion } from 'framer-motion';
import {
  CheckSquare,
  Play,
  Pause,
  RotateCcw,
  XCircle,
  Clock,
  Terminal,
  FileCode,
  Sparkles,
  ExternalLink,
  ChevronRight,
  Filter,
  Search,
  Plus,
  Bot,
  Layers,
  CheckCircle2,
  AlertCircle,
} from 'lucide-react';
import { useVaniStore } from '@/stores/useVaniStore';
import { Task, TaskState } from '@/types';
import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { Drawer } from '@/components/ui/Drawer';
import { Modal } from '@/components/ui/Modal';
import { slideUpFade } from '@/design-system/motion';

export const TasksView: React.FC = () => {
  const { id: paramTaskId } = useParams<{ id: string }>();
  const navigate = useNavigate();
  const tasks = useVaniStore((s) => s.tasks);
  const cancelTask = useVaniStore((s) => s.cancelTask);
  const retryTask = useVaniStore((s) => s.retryTask);
  const addTask = useVaniStore((s) => s.addTask);

  const [activeTab, setActiveTab] = useState<TaskState | 'all'>('all');
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedTask, setSelectedTask] = useState<Task | null>(
    paramTaskId ? tasks.find((t) => t.id === paramTaskId) || null : null
  );
  const [isCreateModalOpen, setIsCreateModalOpen] = useState(false);
  const [newTaskTitle, setNewTaskTitle] = useState('');
  const [newTaskCategory, setNewTaskCategory] = useState<Task['category']>('coding');

  // Filter tasks
  const filteredTasks = tasks.filter((task) => {
    const matchesTab = activeTab === 'all' || task.state === activeTab;
    const matchesSearch =
      task.title.toLowerCase().includes(searchQuery.toLowerCase()) ||
      (task.agentName && task.agentName.toLowerCase().includes(searchQuery.toLowerCase()));
    return matchesTab && matchesSearch;
  });

  const getStatusBadge = (state: TaskState) => {
    switch (state) {
      case 'running':
        return <Badge variant="primary" dot>Running</Badge>;
      case 'completed':
        return <Badge variant="success" dot>Completed</Badge>;
      case 'failed':
        return <Badge variant="danger" dot>Failed</Badge>;
      case 'cancelled':
        return <Badge variant="neutral">Cancelled</Badge>;
      case 'waiting':
        return <Badge variant="warning" dot>Waiting</Badge>;
      default:
        return <Badge variant="outline">Queued</Badge>;
    }
  };

  const handleCreateTask = (e: React.FormEvent) => {
    e.preventDefault();
    if (!newTaskTitle.trim()) return;
    const newId = addTask({
      title: newTaskTitle,
      category: newTaskCategory,
      state: 'running',
      priority: 'high',
      agentName: newTaskCategory === 'coding' ? 'Odysseus' : newTaskCategory === 'research' ? 'Hermes' : 'Ruflo',
      agentAvatar: newTaskCategory === 'coding' ? '⚡' : newTaskCategory === 'research' ? '🧭' : '⚙️',
      description: `Autonomous execution for: ${newTaskTitle}`,
    });
    setNewTaskTitle('');
    setIsCreateModalOpen(false);
    navigate(`/tasks/${newId}`);
  };

  return (
    <motion.div variants={slideUpFade} initial="initial" animate="animate" className="space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl md:text-2xl font-bold text-white tracking-tight">Task Orchestrator</h1>
          <p className="text-xs md:text-sm text-slate-400">
            Real-time execution center for autonomous coding, research, and system agents.
          </p>
        </div>
        <Button
          variant="primary"
          size="sm"
          leftIcon={<Plus className="w-4 h-4" />}
          onClick={() => setIsCreateModalOpen(true)}
        >
          Create Task
        </Button>
      </div>

      {/* Filter Tabs & Search Bar */}
      <div className="flex flex-col md:flex-row items-stretch md:items-center justify-between gap-3">
        {/* Tabs */}
        <div className="flex items-center gap-1.5 p-1 rounded-xl bg-white/[0.03] border border-white/10 overflow-x-auto">
          {(['all', 'running', 'queued', 'waiting', 'completed', 'cancelled'] as const).map((tab) => (
            <button
              key={tab}
              onClick={() => setActiveTab(tab)}
              className={`px-3 py-1.5 rounded-lg text-xs font-medium capitalize transition-all whitespace-nowrap ${
                activeTab === tab
                  ? 'bg-purple-600 text-white shadow-sm'
                  : 'text-slate-400 hover:text-white hover:bg-white/[0.04]'
              }`}
            >
              {tab}
            </button>
          ))}
        </div>

        {/* Search */}
        <div className="relative">
          <Search className="w-3.5 h-3.5 text-slate-400 absolute left-3 top-1/2 -translate-y-1/2" />
          <input
            type="text"
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            placeholder="Search tasks..."
            className="w-full md:w-64 pl-9 pr-3 py-1.5 rounded-xl bg-white/[0.03] border border-white/10 text-xs text-white placeholder:text-slate-400 outline-none focus:border-purple-500/50"
          />
        </div>
      </div>

      {/* Task Cards Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
        {filteredTasks.map((task) => (
          <Card
            key={task.id}
            variant="interactive"
            onClick={() => {
              setSelectedTask(task);
              navigate(`/tasks/${task.id}`);
            }}
            className="space-y-4 flex flex-col justify-between"
          >
            <div className="space-y-2.5">
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-2">
                  <span className="text-lg">{task.agentAvatar || '⚡'}</span>
                  <span className="text-xs font-semibold text-purple-300 font-mono">
                    {task.agentName || 'VANI Core'}
                  </span>
                </div>
                {getStatusBadge(task.state)}
              </div>

              <h3 className="text-sm font-semibold text-white leading-snug line-clamp-2">
                {task.title}
              </h3>

              {task.description && (
                <p className="text-xs text-slate-400 line-clamp-2 leading-relaxed">
                  {task.description}
                </p>
              )}
            </div>

            <div className="space-y-3 pt-2 border-t border-white/5">
              {/* Progress */}
              {task.state === 'running' && (
                <div className="space-y-1">
                  <div className="flex justify-between text-[11px] font-mono text-slate-300">
                    <span>Progress</span>
                    <span className="text-purple-400 font-bold">{task.progress}%</span>
                  </div>
                  <div className="w-full h-1.5 bg-white/10 rounded-full overflow-hidden">
                    <div
                      className="h-full bg-gradient-to-r from-purple-500 to-cyan-400 rounded-full"
                      style={{ width: `${task.progress}%` }}
                    />
                  </div>
                </div>
              )}

              {/* Footer metadata */}
              <div className="flex items-center justify-between text-[11px] text-slate-400 font-mono">
                <span>{task.category}</span>
                <span>{task.startedAt || 'Queued'}</span>
              </div>
            </div>
          </Card>
        ))}
      </div>

      {/* Task Inspector Drawer */}
      <Drawer
        isOpen={!!selectedTask}
        onClose={() => {
          setSelectedTask(null);
          navigate('/tasks');
        }}
        title={selectedTask?.title || 'Task Details'}
        subtitle={`Task ID: ${selectedTask?.id}`}
        width="xl"
      >
        {selectedTask && (
          <div className="space-y-6">
            {/* Status & Controls */}
            <div className="p-4 rounded-xl bg-white/[0.03] border border-white/10 flex items-center justify-between">
              <div className="flex items-center gap-3">
                <span className="text-2xl">{selectedTask.agentAvatar || '⚡'}</span>
                <div>
                  <h4 className="text-xs font-semibold text-white">{selectedTask.agentName}</h4>
                  <span className="text-[11px] text-slate-400 font-mono">
                    Model: {selectedTask.modelUsed || 'Llama 3.8B'}
                  </span>
                </div>
              </div>
              <div className="flex items-center gap-2">
                {selectedTask.state === 'running' ? (
                  <Button
                    variant="danger"
                    size="xs"
                    leftIcon={<XCircle className="w-3.5 h-3.5" />}
                    onClick={() => cancelTask(selectedTask.id)}
                  >
                    Cancel Task
                  </Button>
                ) : (
                  <Button
                    variant="secondary"
                    size="xs"
                    leftIcon={<RotateCcw className="w-3.5 h-3.5" />}
                    onClick={() => retryTask(selectedTask.id)}
                  >
                    Retry
                  </Button>
                )}
              </div>
            </div>

            {/* Description */}
            <div className="space-y-1.5">
              <h4 className="text-xs uppercase font-mono text-slate-400 font-semibold">Objective</h4>
              <p className="text-xs text-slate-200 leading-relaxed bg-white/[0.02] p-3 rounded-lg border border-white/5">
                {selectedTask.description || selectedTask.title}
              </p>
            </div>

            {/* Tools Executed */}
            {selectedTask.toolsUsed && selectedTask.toolsUsed.length > 0 && (
              <div className="space-y-2">
                <h4 className="text-xs uppercase font-mono text-slate-400 font-semibold">
                  Tools Dispatched
                </h4>
                <div className="space-y-2">
                  {selectedTask.toolsUsed.map((tool, idx) => (
                    <div
                      key={idx}
                      className="p-2.5 rounded-lg bg-black/30 border border-white/5 flex flex-col gap-1 text-xs"
                    >
                      <div className="flex items-center justify-between">
                        <span className="font-mono text-purple-300 font-semibold flex items-center gap-1.5">
                          <Terminal className="w-3.5 h-3.5 text-cyan-400" />
                          {tool.name}
                        </span>
                        <Badge variant="cyan" size="xs">{tool.status}</Badge>
                      </div>
                      {tool.outputSnippet && (
                        <p className="font-mono text-[11px] text-slate-400 bg-white/[0.02] p-1.5 rounded">
                          {tool.outputSnippet}
                        </p>
                      )}
                    </div>
                  ))}
                </div>
              </div>
            )}

            {/* Artifacts Created */}
            {selectedTask.artifacts && selectedTask.artifacts.length > 0 && (
              <div className="space-y-2">
                <h4 className="text-xs uppercase font-mono text-slate-400 font-semibold">
                  Artifacts Generated
                </h4>
                <div className="space-y-2">
                  {selectedTask.artifacts.map((art) => (
                    <div
                      key={art.id}
                      className="p-3 rounded-xl bg-purple-950/20 border border-purple-500/30 space-y-2 text-xs"
                    >
                      <div className="flex items-center justify-between">
                        <span className="font-mono text-white font-semibold flex items-center gap-2">
                          <FileCode className="w-4 h-4 text-purple-400" />
                          {art.name}
                        </span>
                        <span className="text-[10px] text-slate-400 font-mono">{art.size}</span>
                      </div>
                      {art.content && (
                        <pre className="p-2.5 rounded-lg bg-black/50 overflow-x-auto font-mono text-[11px] text-slate-300">
                          <code>{art.content}</code>
                        </pre>
                      )}
                    </div>
                  ))}
                </div>
              </div>
            )}

            {/* Execution Timeline */}
            {selectedTask.timeline && (
              <div className="space-y-2">
                <h4 className="text-xs uppercase font-mono text-slate-400 font-semibold">
                  Execution Timeline
                </h4>
                <div className="space-y-2 border-l border-white/10 pl-3 ml-1 text-xs">
                  {selectedTask.timeline.map((item) => (
                    <div key={item.id} className="relative pl-3">
                      <span className="absolute -left-[19px] top-1 w-2 h-2 rounded-full bg-purple-400" />
                      <span className="text-[10px] text-slate-400 font-mono">{item.timestamp}</span>
                      <p className="text-slate-200">{item.description}</p>
                    </div>
                  ))}
                </div>
              </div>
            )}

            {/* Raw Logs */}
            {selectedTask.logs && (
              <div className="space-y-2">
                <h4 className="text-xs uppercase font-mono text-slate-400 font-semibold">Live Logs</h4>
                <div className="p-3 rounded-xl bg-black border border-white/10 font-mono text-[11px] text-emerald-400 space-y-1">
                  {selectedTask.logs.map((log, i) => (
                    <div key={i}>{log}</div>
                  ))}
                </div>
              </div>
            )}
          </div>
        )}
      </Drawer>

      {/* Create Task Modal */}
      <Modal
        isOpen={isCreateModalOpen}
        onClose={() => setIsCreateModalOpen(false)}
        title="Create New System Task"
        subtitle="Assign an autonomous goal to VANI's local agent layer"
      >
        <form onSubmit={handleCreateTask} className="space-y-4">
          <div>
            <label className="text-xs font-mono text-slate-400 block mb-1.5">Task Objective</label>
            <input
              type="text"
              autoFocus
              value={newTaskTitle}
              onChange={(e) => setNewTaskTitle(e.target.value)}
              placeholder="e.g. Build C++ To-Do engine or Research Agent Papers"
              className="w-full px-3 py-2 rounded-xl bg-white/[0.04] border border-white/10 text-sm text-white placeholder:text-slate-500 outline-none focus:border-purple-500/50"
            />
          </div>

          <div>
            <label className="text-xs font-mono text-slate-400 block mb-1.5">Category</label>
            <div className="grid grid-cols-3 gap-2">
              {(['coding', 'research', 'automation'] as const).map((cat) => (
                <button
                  type="button"
                  key={cat}
                  onClick={() => setNewTaskCategory(cat)}
                  className={`py-2 rounded-xl text-xs font-semibold capitalize border transition-all ${
                    newTaskCategory === cat
                      ? 'bg-purple-600/30 border-purple-500 text-white'
                      : 'bg-white/[0.03] border-white/10 text-slate-400 hover:text-white'
                  }`}
                >
                  {cat}
                </button>
              ))}
            </div>
          </div>

          <div className="flex justify-end gap-2 pt-3 border-t border-white/10">
            <Button variant="ghost" size="sm" type="button" onClick={() => setIsCreateModalOpen(false)}>
              Cancel
            </Button>
            <Button variant="primary" size="sm" type="submit" disabled={!newTaskTitle.trim()}>
              Launch Task
            </Button>
          </div>
        </form>
      </Modal>
    </motion.div>
  );
};
