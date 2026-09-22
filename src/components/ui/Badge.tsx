import React from 'react';
import { clsx } from 'clsx';
import { twMerge } from 'tailwind-merge';

export interface BadgeProps extends React.HTMLAttributes<HTMLSpanElement> {
  variant?: 'default' | 'primary' | 'cyan' | 'success' | 'warning' | 'danger' | 'outline' | 'neutral';
  size?: 'xs' | 'sm' | 'md';
  dot?: boolean;
}

export const Badge: React.FC<BadgeProps> = ({
  className,
  variant = 'default',
  size = 'sm',
  dot = false,
  children,
  ...props
}) => {
  const sizeStyles = {
    xs: 'px-1.5 py-0.5 text-[10px] gap-1',
    sm: 'px-2.5 py-0.5 text-xs gap-1.5',
    md: 'px-3 py-1 text-xs gap-2',
  };

  const variantStyles = {
    default: 'bg-white/[0.06] text-slate-300 border border-white/10',
    primary: 'bg-purple-500/15 text-purple-300 border border-purple-500/30',
    cyan: 'bg-cyan-500/15 text-cyan-300 border border-cyan-500/30',
    success: 'bg-emerald-500/15 text-emerald-300 border border-emerald-500/30',
    warning: 'bg-amber-500/15 text-amber-300 border border-amber-500/30',
    danger: 'bg-rose-500/15 text-rose-300 border border-rose-500/30',
    outline: 'bg-transparent text-slate-300 border border-white/20',
    neutral: 'bg-slate-800 text-slate-400 border border-slate-700/50',
  };

  const dotColors = {
    default: 'bg-slate-400',
    primary: 'bg-purple-400 shadow-[0_0_8px_#a855f7]',
    cyan: 'bg-cyan-400 shadow-[0_0_8px_#06b6d4]',
    success: 'bg-emerald-400 shadow-[0_0_8px_#10b981]',
    warning: 'bg-amber-400 shadow-[0_0_8px_#f59e0b]',
    danger: 'bg-rose-400 shadow-[0_0_8px_#f43f5e]',
    outline: 'bg-slate-300',
    neutral: 'bg-slate-500',
  };

  return (
    <span
      className={twMerge(
        clsx(
          'inline-flex items-center font-medium rounded-full select-none',
          sizeStyles[size],
          variantStyles[variant],
          className
        )
      )}
      {...props}
    >
      {dot && (
        <span className={clsx('w-1.5 h-1.5 rounded-full flex-shrink-0', dotColors[variant])} />
      )}
      {children}
    </span>
  );
};
