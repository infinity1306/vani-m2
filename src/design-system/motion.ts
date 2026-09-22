import { Variants, Transition } from 'framer-motion';

export const springSlow: Transition = {
  type: 'spring',
  stiffness: 180,
  damping: 24,
};

export const springFast: Transition = {
  type: 'spring',
  stiffness: 300,
  damping: 28,
};

export const easeEditorial: Transition = {
  duration: 0.22,
  ease: [0.16, 1, 0.3, 1],
};

export const fadeInScale: Variants = {
  initial: { opacity: 0, scale: 0.96 },
  animate: { opacity: 1, scale: 1, transition: easeEditorial },
  exit: { opacity: 0, scale: 0.96, transition: { duration: 0.15, ease: 'easeIn' } },
};

export const slideUpFade: Variants = {
  initial: { opacity: 0, y: 12 },
  animate: { opacity: 1, y: 0, transition: easeEditorial },
  exit: { opacity: 0, y: -8, transition: { duration: 0.15 } },
};

export const slideRightFade: Variants = {
  initial: { opacity: 0, x: -16 },
  animate: { opacity: 1, x: 0, transition: easeEditorial },
  exit: { opacity: 0, x: 16, transition: { duration: 0.15 } },
};

export const staggerContainer: Variants = {
  initial: {},
  animate: {
    transition: {
      staggerChildren: 0.04,
    },
  },
};

export const itemFadeIn: Variants = {
  initial: { opacity: 0, y: 8 },
  animate: { opacity: 1, y: 0, transition: { duration: 0.2, ease: [0.16, 1, 0.3, 1] } },
};

export const orbPulseVariants: Variants = {
  idle: {
    scale: [1, 1.03, 1],
    opacity: [0.85, 0.95, 0.85],
    transition: { duration: 3.2, repeat: Infinity, ease: 'easeInOut' },
  },
  listening: {
    scale: [1, 1.08, 0.98, 1.06, 1],
    opacity: 1,
    transition: { duration: 1.6, repeat: Infinity, ease: 'easeInOut' },
  },
  thinking: {
    rotate: 360,
    scale: [0.98, 1.02, 0.98],
    transition: { rotate: { duration: 3, repeat: Infinity, ease: 'linear' }, scale: { duration: 2, repeat: Infinity } },
  },
  executing: {
    scale: [1, 1.05, 1],
    transition: { duration: 0.8, repeat: Infinity, ease: 'easeInOut' },
  },
  speaking: {
    scale: [1, 1.1, 0.96, 1.08, 1],
    transition: { duration: 1.2, repeat: Infinity, ease: 'easeInOut' },
  },
  waiting_confirmation: {
    scale: [1, 1.04, 1],
    transition: { duration: 2, repeat: Infinity },
  },
  error: {
    scale: [1, 1.02, 1],
    x: [0, -3, 3, -2, 2, 0],
    transition: { duration: 0.6, repeat: Infinity, repeatDelay: 2 },
  },
  offline: {
    scale: 0.95,
    opacity: 0.6,
    transition: { duration: 0.4 },
  },
};
