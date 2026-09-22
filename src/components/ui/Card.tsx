import React from 'react';
import { clsx } from 'clsx';
import { twMerge } from 'tailwind-merge';

export interface CardProps extends React.HTMLAttributes<HTMLDivElement> {
  variant?: 'default' | 'elevated' | 'interactive' | 'outline' | 'glow' | 'danger' | 'warning';
  noPadding?: boolean;
}

export const Card: React.FC<CardProps> = ({
  className,
  variant = 'default',
  noPadding = false,
  children,
  ...props
}) => {
  const baseStyles = 'rounded-xl transition-all duration-200';

  const variantStyles = {
    default: 'vani-glass-card',
    elevated: 'vani-glass-elevated',
    interactive: 'vani-glass-card hover:translate-y-[-2px] hover:border-purple-500/40 hover:shadow-lg hover:shadow-purple-900/15 cursor-pointer',
    outline: 'bg-transparent border border-white/10 hover:border-white/20',
    glow: 'vani-glass border-purple-500/30 shadow-lg shadow-purple-900/20',
    danger: 'bg-rose-950/20 border border-rose-500/30 text-rose-200',
    warning: 'bg-amber-950/20 border border-amber-500/30 text-amber-200',
  };

  return (
    <div
      className={twMerge(
        clsx(baseStyles, variantStyles[variant], !noPadding && 'p-4 md:p-5', className)
      )}
      {...props}
    >
      {children}
    </div>
  );
};
