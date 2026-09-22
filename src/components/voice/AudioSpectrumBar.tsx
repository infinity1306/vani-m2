import React from 'react';
import { motion } from 'framer-motion';

interface AudioSpectrumBarProps {
  levels?: number[];
  isActive?: boolean;
  barCount?: number;
  color?: 'purple' | 'cyan' | 'emerald';
}

export const AudioSpectrumBar: React.FC<AudioSpectrumBarProps> = ({
  levels = [15, 35, 65, 45, 85, 95, 70, 50, 80, 40, 25, 60, 30],
  isActive = true,
  barCount = 12,
  color = 'purple',
}) => {
  const gradientClass = {
    purple: 'from-purple-500 via-indigo-400 to-cyan-400',
    cyan: 'from-cyan-500 to-blue-400',
    emerald: 'from-emerald-500 to-teal-300',
  }[color];

  return (
    <div className="flex items-center gap-1 h-6 px-1">
      {Array.from({ length: barCount }).map((_, i) => {
        const heightPercent = isActive ? levels[i % levels.length] : 15;
        return (
          <motion.div
            key={i}
            animate={
              isActive
                ? {
                    height: [`${Math.max(15, heightPercent * 0.3)}%`, `${heightPercent}%`, `${Math.max(20, heightPercent * 0.5)}%`],
                  }
                : { height: '15%' }
            }
            transition={{
              duration: 0.5 + (i % 3) * 0.15,
              repeat: Infinity,
              ease: 'easeInOut',
            }}
            className={`w-1 rounded-full bg-gradient-to-t ${gradientClass} opacity-90`}
          />
        );
      })}
    </div>
  );
};
