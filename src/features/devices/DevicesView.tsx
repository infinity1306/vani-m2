import React from 'react';
import { motion } from 'framer-motion';
import {
  Smartphone,
  Laptop,
  Headphones,
  Watch,
  Battery,
  Wifi,
  Mic,
  Camera,
  Shield,
  Activity,
  Plus,
  RefreshCw,
  Zap,
} from 'lucide-react';
import { useVaniStore } from '@/stores/useVaniStore';
import { ConnectedDevice } from '@/types';
import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { slideUpFade } from '@/design-system/motion';

export const DevicesView: React.FC = () => {
  const devices = useVaniStore((s) => s.devices);
  const toggleDeviceConnection = useVaniStore((s) => s.toggleDeviceConnection);

  const getDeviceIcon = (type: ConnectedDevice['type']) => {
    switch (type) {
      case 'computer':
        return <Laptop className="w-5 h-5 text-purple-400" />;
      case 'phone':
        return <Smartphone className="w-5 h-5 text-cyan-400" />;
      case 'buds':
        return <Headphones className="w-5 h-5 text-emerald-400" />;
      case 'watch':
        return <Watch className="w-5 h-5 text-amber-400" />;
      default:
        return <Zap className="w-5 h-5 text-purple-400" />;
    }
  };

  return (
    <motion.div variants={slideUpFade} initial="initial" animate="animate" className="space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl md:text-2xl font-bold text-white tracking-tight">Connected Ecosystem & Devices</h1>
          <p className="text-xs md:text-sm text-slate-400">
            Spatial audio, multi-mic beamforming, camera feeds, and distributed compute across your hardware.
          </p>
        </div>
        <Button variant="primary" size="sm" leftIcon={<Plus className="w-4 h-4" />}>
          Pair New Device
        </Button>
      </div>

      {/* Hardware Topology Visualizer */}
      <Card variant="glow" className="space-y-4">
        <h3 className="text-xs font-semibold uppercase font-mono text-slate-400">
          Hardware Mesh Topology (LE Direct Bridge)
        </h3>
        <div className="relative p-6 rounded-xl bg-black/40 border border-white/5 flex flex-wrap items-center justify-around gap-6">
          {devices.map((dev) => (
            <div key={dev.id} className="flex flex-col items-center gap-2 group">
              <div
                className={`w-14 h-14 rounded-2xl border flex items-center justify-center transition-all ${
                  dev.status === 'connected'
                    ? 'bg-purple-950/40 border-purple-500/50 shadow-[0_0_20px_rgba(139,92,246,0.3)]'
                    : 'bg-white/[0.02] border-white/10 opacity-40'
                }`}
              >
                {getDeviceIcon(dev.type)}
              </div>
              <span className="text-xs font-semibold text-white truncate max-w-[120px] text-center">
                {dev.name.split(' ')[0]}
              </span>
              <span className="text-[10px] font-mono text-purple-300">
                {dev.latencyMs} ms
              </span>
            </div>
          ))}
        </div>
      </Card>

      {/* Devices List */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-5">
        {devices.map((device) => (
          <Card key={device.id} className="space-y-4">
            <div className="flex items-start justify-between">
              <div className="flex items-center gap-3">
                <div className="w-10 h-10 rounded-xl bg-white/[0.04] border border-white/10 flex items-center justify-center">
                  {getDeviceIcon(device.type)}
                </div>
                <div>
                  <h3 className="text-sm font-bold text-white">{device.name}</h3>
                  <span className="text-xs text-slate-400 font-mono">{device.modelInfo}</span>
                </div>
              </div>
              <Badge
                variant={device.status === 'connected' ? 'success' : 'neutral'}
                dot={device.status === 'connected'}
                size="xs"
              >
                {device.status}
              </Badge>
            </div>

            {/* Hardware capabilities chips */}
            <div className="grid grid-cols-2 sm:grid-cols-3 gap-2 text-xs font-mono">
              <div className="p-2 rounded-lg bg-black/30 border border-white/5 flex items-center justify-between">
                <span className="text-slate-400 flex items-center gap-1">
                  <Battery className="w-3.5 h-3.5 text-emerald-400" /> Battery
                </span>
                <span className="text-white font-bold">{device.batteryPercentage ?? '—'}%</span>
              </div>
              <div className="p-2 rounded-lg bg-black/30 border border-white/5 flex items-center justify-between">
                <span className="text-slate-400 flex items-center gap-1">
                  <Wifi className="w-3.5 h-3.5 text-cyan-400" /> Latency
                </span>
                <span className="text-white font-bold">{device.latencyMs}ms</span>
              </div>
              <div className="p-2 rounded-lg bg-black/30 border border-white/5 flex items-center justify-between">
                <span className="text-slate-400 flex items-center gap-1">
                  <Mic className="w-3.5 h-3.5 text-purple-400" /> Mic
                </span>
                <span className={device.hasMicrophone ? 'text-emerald-400' : 'text-slate-500'}>
                  {device.hasMicrophone ? 'Avail' : 'None'}
                </span>
              </div>
            </div>

            {/* Permissions */}
            <div className="space-y-1">
              <span className="text-[10px] uppercase font-mono text-slate-400">Active Relays:</span>
              <div className="flex flex-wrap gap-1">
                {device.permissions.map((p, i) => (
                  <span key={i} className="text-[10px] font-mono px-2 py-0.5 rounded bg-white/[0.04] text-slate-300">
                    {p}
                  </span>
                ))}
              </div>
            </div>

            {/* Toggle footer */}
            <div className="pt-3 border-t border-white/5 flex items-center justify-between text-xs">
              <span className="text-slate-400 text-[11px] font-mono">Last seen: {device.lastSeen}</span>
              <Button
                variant={device.status === 'connected' ? 'secondary' : 'primary'}
                size="xs"
                onClick={() => toggleDeviceConnection(device.id)}
              >
                {device.status === 'connected' ? 'Disconnect' : 'Connect'}
              </Button>
            </div>
          </Card>
        ))}
      </div>
    </motion.div>
  );
};
