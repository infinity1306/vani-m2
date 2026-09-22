import React, { useState } from 'react';
import { motion } from 'framer-motion';
import {
  Shield,
  ShieldAlert,
  ShieldCheck,
  CheckCircle2,
  XCircle,
  AlertTriangle,
  Lock,
  Key,
  Terminal,
  FileCode,
  EyeOff,
  Clock,
  History,
} from 'lucide-react';
import { useVaniStore } from '@/stores/useVaniStore';
import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { slideUpFade } from '@/design-system/motion';

export const SecurityView: React.FC = () => {
  const permissions = useVaniStore((s) => s.permissions);
  const allowPermission = useVaniStore((s) => s.allowPermission);
  const denyPermission = useVaniStore((s) => s.denyPermission);

  const pendingPermissions = permissions.filter((p) => p.status === 'pending');
  const pastAuditLog = permissions.filter((p) => p.status !== 'pending');

  return (
    <motion.div variants={slideUpFade} initial="initial" animate="animate" className="space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl md:text-2xl font-bold text-white tracking-tight">Security & Sandboxing Gate</h1>
          <p className="text-xs md:text-sm text-slate-400">
            Hardware-enforced guardrails, permission review queues, secrets redaction, and execution audit log.
          </p>
        </div>
        <Badge variant="success" dot size="sm">
          Aegis Guard Active
        </Badge>
      </div>

      {/* Security Status Overview */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        <Card variant="elevated" className="space-y-2">
          <div className="flex items-center gap-2 text-emerald-400">
            <ShieldCheck className="w-5 h-5" />
            <h3 className="text-sm font-semibold text-white">Capability Isolation</h3>
          </div>
          <p className="text-xs text-slate-300">
            Agents run inside isolated micro-containers with non-root user permissions.
          </p>
        </Card>

        <Card variant="elevated" className="space-y-2">
          <div className="flex items-center gap-2 text-purple-400">
            <Lock className="w-5 h-5" />
            <h3 className="text-sm font-semibold text-white">Local Secret Vault</h3>
          </div>
          <p className="text-xs text-slate-300">
            API keys and tokens are encrypted with AES-256-GCM in OS secure enclave.
          </p>
        </Card>

        <Card variant="elevated" className="space-y-2">
          <div className="flex items-center gap-2 text-cyan-400">
            <EyeOff className="w-5 h-5" />
            <h3 className="text-sm font-semibold text-white">Privacy Screen Masking</h3>
          </div>
          <p className="text-xs text-slate-300">
            Passphrases, personal emails, and banking tokens are redacted before vision models process screen frames.
          </p>
        </Card>
      </div>

      {/* Pending Permission Review Queue */}
      <div className="space-y-3">
        <div className="flex items-center justify-between">
          <h3 className="text-sm font-bold text-white flex items-center gap-2">
            <ShieldAlert className="w-4 h-4 text-amber-400" />
            <span>Pending Permission Requests ({pendingPermissions.length})</span>
          </h3>
          <span className="text-xs text-slate-400 font-mono">Explicit Confirmation Required</span>
        </div>

        {pendingPermissions.length === 0 ? (
          <Card className="py-8 text-center text-slate-400 text-xs font-mono">
            <CheckCircle2 className="w-6 h-6 text-emerald-400 mx-auto mb-2 opacity-80" />
            No pending elevation requests. All operations operating within authorized bounds.
          </Card>
        ) : (
          pendingPermissions.map((perm) => (
            <Card
              key={perm.id}
              variant="warning"
              className="p-5 space-y-3 border-amber-500/40 bg-amber-950/20"
            >
              <div className="flex items-start justify-between">
                <div>
                  <div className="flex items-center gap-2">
                    <Badge variant="warning" size="xs">
                      {perm.riskLevel.toUpperCase()} RISK
                    </Badge>
                    <span className="text-xs text-slate-400 font-mono">{perm.timestamp}</span>
                  </div>
                  <h4 className="text-sm font-bold text-white mt-1">{perm.title}</h4>
                  <p className="text-xs text-amber-100/90 mt-0.5 leading-relaxed">{perm.description}</p>
                </div>
              </div>

              {perm.details && (
                <div className="p-3 rounded-xl bg-black/50 border border-white/10 font-mono text-xs text-slate-300 space-y-1">
                  <div>
                    <span className="text-slate-500">Source: </span>
                    <strong className="text-purple-300">{perm.sourceAgent} ({perm.sourceTool})</strong>
                  </div>
                  {perm.details.command && (
                    <div>
                      <span className="text-slate-500">Command: </span>
                      <code className="text-amber-300">{perm.details.command}</code>
                    </div>
                  )}
                  {perm.details.workingDir && (
                    <div>
                      <span className="text-slate-500">Directory: </span>
                      <span>{perm.details.workingDir}</span>
                    </div>
                  )}
                </div>
              )}

              <div className="flex justify-end gap-2 pt-2 border-t border-amber-500/20">
                <Button
                  variant="danger"
                  size="xs"
                  leftIcon={<XCircle className="w-3.5 h-3.5" />}
                  onClick={() => denyPermission(perm.id)}
                >
                  Deny Execution
                </Button>
                <Button
                  variant="primary"
                  size="xs"
                  leftIcon={<CheckCircle2 className="w-3.5 h-3.5" />}
                  onClick={() => allowPermission(perm.id)}
                >
                  Allow Once
                </Button>
              </div>
            </Card>
          ))
        )}
      </div>

      {/* Audit Log */}
      <div className="space-y-3">
        <h3 className="text-sm font-bold text-white flex items-center gap-2">
          <History className="w-4 h-4 text-purple-400" />
          <span>Security Audit Trail</span>
        </h3>

        <Card className="p-0 overflow-hidden">
          <div className="overflow-x-auto">
            <table className="w-full text-left text-xs">
              <thead className="bg-white/[0.03] border-b border-white/10 text-slate-400 font-mono">
                <tr>
                  <th className="p-3">Timestamp</th>
                  <th className="p-3">Event / Operation</th>
                  <th className="p-3">Source</th>
                  <th className="p-3">Risk Level</th>
                  <th className="p-3 text-right">Verdict</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-white/5 font-mono text-slate-300">
                {pastAuditLog.map((log) => (
                  <tr key={log.id} className="hover:bg-white/[0.02] transition-colors">
                    <td className="p-3 text-slate-400">{log.timestamp}</td>
                    <td className="p-3 text-white font-medium">{log.title}</td>
                    <td className="p-3 text-purple-300">{log.sourceAgent}</td>
                    <td className="p-3">
                      <span className="capitalize">{log.riskLevel}</span>
                    </td>
                    <td className="p-3 text-right">
                      <Badge variant={log.status === 'allowed' ? 'success' : 'danger'} size="xs">
                        {log.status.toUpperCase()}
                      </Badge>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </Card>
      </div>
    </motion.div>
  );
};
