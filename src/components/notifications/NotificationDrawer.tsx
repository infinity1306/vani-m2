import React from 'react';
import { Drawer } from '@/components/ui/Drawer';
import { useVaniStore } from '@/stores/useVaniStore';
import { Bell, Check, Trash2, CheckCircle2, AlertTriangle, AlertCircle, Info } from 'lucide-react';
import { Button } from '@/components/ui/Button';

interface NotificationDrawerProps {
  isOpen: boolean;
  onClose: () => void;
}

export const NotificationDrawer: React.FC<NotificationDrawerProps> = ({ isOpen, onClose }) => {
  const notifications = useVaniStore((s) => s.notifications);
  const markAsRead = useVaniStore((s) => s.markNotificationAsRead);
  const clearAll = useVaniStore((s) => s.clearAllNotifications);

  const getIcon = (type: string) => {
    switch (type) {
      case 'success':
        return <CheckCircle2 className="w-4 h-4 text-emerald-400" />;
      case 'warning':
        return <AlertTriangle className="w-4 h-4 text-amber-400" />;
      case 'error':
        return <AlertCircle className="w-4 h-4 text-rose-400" />;
      default:
        return <Info className="w-4 h-4 text-purple-400" />;
    }
  };

  return (
    <Drawer
      isOpen={isOpen}
      onClose={onClose}
      title="System Notifications"
      subtitle={`${notifications.filter((n) => !n.read).length} unread updates`}
      width="md"
    >
      <div className="flex items-center justify-between pb-2 border-b border-white/5">
        <span className="text-xs text-slate-400">Activity Stream</span>
        {notifications.length > 0 && (
          <button
            onClick={clearAll}
            className="text-xs text-slate-500 hover:text-slate-300 flex items-center gap-1 transition-colors"
          >
            <Trash2 className="w-3.5 h-3.5" />
            Clear all
          </button>
        )}
      </div>

      <div className="space-y-3">
        {notifications.length === 0 ? (
          <div className="py-16 text-center text-slate-500 text-sm">
            <Bell className="w-8 h-8 mx-auto mb-2 opacity-30" />
            No new notifications. All systems optimal.
          </div>
        ) : (
          notifications.map((notif) => (
            <div
              key={notif.id}
              onClick={() => markAsRead(notif.id)}
              className={`p-3.5 rounded-xl border transition-all cursor-pointer ${
                notif.read
                  ? 'bg-white/[0.02] border-white/5 text-slate-400'
                  : 'bg-purple-950/20 border-purple-500/30 text-slate-200'
              }`}
            >
              <div className="flex items-start gap-3">
                <div className="p-1.5 rounded-lg bg-white/[0.05] flex-shrink-0 mt-0.5">
                  {getIcon(notif.type)}
                </div>
                <div className="flex-1 min-w-0">
                  <div className="flex items-center justify-between">
                    <h4 className="text-xs font-semibold text-white truncate">{notif.title}</h4>
                    <span className="text-[10px] text-slate-500 font-mono">{notif.timestamp}</span>
                  </div>
                  <p className="text-xs text-slate-300 mt-1 leading-relaxed">{notif.message}</p>
                </div>
              </div>
            </div>
          ))
        )}
      </div>
    </Drawer>
  );
};
