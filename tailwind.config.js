/** @type {import('tailwindcss').Config} */
export default {
  darkMode: ['class'],
  content: [
    './index.html',
    './src/**/*.{js,ts,jsx,tsx}',
  ],
  theme: {
    extend: {
      colors: {
        border: 'hsl(var(--border))',
        input: 'hsl(var(--input))',
        ring: 'hsl(var(--ring))',
        background: 'hsl(var(--background))',
        foreground: 'hsl(var(--foreground))',
        vani: {
          darkest: '#05070b',
          surface: '#090d16',
          elevated: '#0f172a',
          card: '#111827',
          panel: '#151e30',
          border: 'rgba(255, 255, 255, 0.08)',
          borderGlow: 'rgba(124, 58, 237, 0.25)',
          primary: '#8b5cf6',
          primaryHover: '#7c3aed',
          primaryGlow: 'rgba(139, 92, 246, 0.35)',
          cyan: '#06b6d4',
          cyanGlow: 'rgba(6, 182, 212, 0.35)',
          emerald: '#10b981',
          emeraldGlow: 'rgba(16, 185, 129, 0.3)',
          amber: '#f59e0b',
          rose: '#f43f5e',
          textMuted: '#94a3b8',
          textBright: '#f8fafc',
        },
        primary: {
          DEFAULT: 'hsl(var(--primary))',
          foreground: 'hsl(var(--primary-foreground))',
        },
        secondary: {
          DEFAULT: 'hsl(var(--secondary))',
          foreground: 'hsl(var(--secondary-foreground))',
        },
        destructive: {
          DEFAULT: 'hsl(var(--destructive))',
          foreground: 'hsl(var(--destructive-foreground))',
        },
        muted: {
          DEFAULT: 'hsl(var(--muted))',
          foreground: 'hsl(var(--muted-foreground))',
        },
        accent: {
          DEFAULT: 'hsl(var(--accent))',
          foreground: 'hsl(var(--accent-foreground))',
        },
        popover: {
          DEFAULT: 'hsl(var(--popover))',
          foreground: 'hsl(var(--popover-foreground))',
        },
        card: {
          DEFAULT: 'hsl(var(--card))',
          foreground: 'hsl(var(--card-foreground))',
        },
      },
      borderRadius: {
        lg: 'var(--radius)',
        md: 'calc(var(--radius) - 2px)',
        sm: 'calc(var(--radius) - 4px)',
      },
      fontFamily: {
        sans: ['Plus Jakarta Sans', 'Inter', '-apple-system', 'BlinkMacSystemFont', 'sans-serif'],
        mono: ['JetBrains Mono', 'Fira Code', 'monospace'],
      },
      animation: {
        'pulse-subtle': 'pulseSubtle 3s cubic-bezier(0.4, 0, 0.6, 1) infinite',
        'orb-spin': 'orbSpin 20s linear infinite',
        'orb-spin-reverse': 'orbSpinRev 25s linear infinite',
        'wave-bar': 'waveBar 1.2s ease-in-out infinite',
      },
      keyframes: {
        pulseSubtle: {
          '0%, 100%': { opacity: '1', transform: 'scale(1)' },
          '50%': { opacity: '0.85', transform: 'scale(1.02)' },
        },
        orbSpin: {
          '0%': { transform: 'rotate(0deg)' },
          '100%': { transform: 'rotate(360deg)' },
        },
        orbSpinRev: {
          '0%': { transform: 'rotate(360deg)' },
          '100%': { transform: 'rotate(0deg)' },
        },
        waveBar: {
          '0%, 100%': { height: '8px' },
          '50%': { height: '36px' },
        },
      },
    },
  },
  plugins: [],
}
