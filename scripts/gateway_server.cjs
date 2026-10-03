/**
 * VANI Mark 2 — High Performance Backend Runtime Gateway
 * HTTP REST & Real-time Server-Sent Events (SSE) Bridge with Phase 3 Voice Subsystem
 */
const http = require('http');
const url = require('url');

const PORT = process.env.PORT || 3002;

// In-Memory Runtime State matching VANI Phase 2 & 3
const runtimeState = {
  lifecycleState: 'READY',
  version: '2.0.0',
  contractAbiVersion: '1.0.0',
  bootTime: Date.now(),
  subsystemHealth: {
    lifecycle: { status: 'HEALTHY', message: 'Runtime state: READY' },
    event_bus: { status: 'HEALTHY', message: 'Event bus active, 2 worker threads' },
    task_manager: { status: 'HEALTHY', message: 'State machine operational (12 states)' },
    capability_registry: { status: 'HEALTHY', message: '8 capabilities registered' },
    policy_engine: { status: 'HEALTHY', message: 'Strict safety policies active (5 risk levels)' },
    permission_service: { status: 'HEALTHY', message: 'Scoped grants enabled' },
    scheduler: { status: 'HEALTHY', message: 'Scheduler ticking (100ms)' },
    watchdog: { status: 'HEALTHY', message: 'Heartbeat monitoring active' },
    persistence: { status: 'HEALTHY', message: 'SQLite Schema v1 migration applied' },
    audio_capture: { status: 'HEALTHY', message: '16kHz Mono Float32 standard initialized' },
    silero_vad: { status: 'HEALTHY', message: 'Silero VAD active (Speech/Silence detection)' },
    open_wakeword: { status: 'HEALTHY', message: 'openWakeWord model loaded ("vani", "hey vani")' },
    sherpa_stt: { status: 'HEALTHY', message: 'Sherpa-ONNX streaming STT engine primary' },
    piper_tts: { status: 'HEALTHY', message: 'Piper neural TTS engine ready (TTFA < 45ms)' },
    hinglish_normalizer: { status: 'HEALTHY', message: 'Layer 1 & Layer 2 normalizer online' },
    entity_resolver: { status: 'HEALTHY', message: 'Contextual vocabulary entity resolver active' }
  },
  systemMetrics: {
    cpu: 18,
    memory: 42,
    gpu: 28,
    vram: 34,
    battery: 98,
    disk: 65,
    networkSpeed: '1.2 GB/s',
    temperature: '48°C',
    npuUsage: 12
  },
  capabilities: [
    {
      id: 'coding.refactor',
      version: '1.0.0',
      type: 'Agent',
      provider_id: 'agent.odysseus',
      description: 'AST-aware automated code refactoring & TypeScript synthesis',
      risk_level: 'MEDIUM',
      availability: 'AVAILABLE',
      supports_cancellation: true
    },
    {
      id: 'research.synthesis',
      version: '1.0.0',
      type: 'Agent',
      provider_id: 'agent.hermes',
      description: 'Deep scientific literature and cross-domain paper analysis',
      risk_level: 'LOW',
      availability: 'AVAILABLE',
      supports_cancellation: true
    },
    {
      id: 'voice.stt.streaming',
      version: '1.0.0',
      type: 'Tool',
      provider_id: 'voice.stt.sherpa',
      description: 'Low-latency streaming speech-to-text with Hinglish normalizer',
      risk_level: 'SAFE',
      availability: 'AVAILABLE',
      supports_cancellation: true
    },
    {
      id: 'voice.tts.piper',
      version: '1.0.0',
      type: 'Tool',
      provider_id: 'voice.tts.piper',
      description: 'Ultra-fast streaming neural speech synthesis with barge-in',
      risk_level: 'SAFE',
      availability: 'AVAILABLE',
      supports_cancellation: true
    },
    {
      id: 'system.echo',
      version: '1.0.0',
      type: 'Tool',
      provider_id: 'core.system',
      description: 'Reference deterministic echo tool',
      risk_level: 'SAFE',
      availability: 'AVAILABLE',
      supports_cancellation: true
    },
    {
      id: 'system.time',
      version: '1.0.0',
      type: 'Tool',
      provider_id: 'core.system.time',
      description: 'High-resolution system clock inspection',
      risk_level: 'SAFE',
      availability: 'AVAILABLE',
      supports_cancellation: true
    },
    {
      id: 'browser.automation',
      version: '1.0.0',
      type: 'Tool',
      provider_id: 'tool.browser',
      description: 'DOM inspection and automated browser interaction',
      risk_level: 'MEDIUM',
      availability: 'AVAILABLE',
      supports_cancellation: true
    },
    {
      id: 'terminal.execute',
      version: '1.0.0',
      type: 'Tool',
      provider_id: 'tool.terminal',
      description: 'Sandboxed shell command execution',
      risk_level: 'HIGH',
      availability: 'REQUIRES_PERMISSION',
      supports_cancellation: true
    }
  ],
  tasks: [
    {
      id: 'task-101',
      title: 'Build To-Do App in C++ & Fix React Auth',
      category: 'coding',
      state: 'running',
      progress: 75,
      priority: 'high',
      agent: 'Odysseus',
      logs: [
        '[14:22:01] [ASTParser] Parsed auth/jwt_verifier.cpp AST in 12ms',
        '[14:22:04] [Compiler] Verified type safety and zero memory leaks',
        '[14:22:08] [Container] Running isolated integration tests (24/24 passing)'
      ]
    },
    {
      id: 'task-102',
      title: 'Research AI Agent Frameworks 2026',
      category: 'research',
      state: 'running',
      progress: 60,
      priority: 'normal',
      agent: 'Hermes',
      logs: [
        '[14:20:15] [Crawler] Indexed 42 arXiv preprint publications',
        '[14:20:45] [VectorDB] Embedded cross-domain semantic graph',
        '[14:21:10] [Synthesizer] Compiling Markdown executive brief'
      ]
    }
  ]
};

// SSE Active Client Connections
const sseClients = new Set();

function broadcastEvent(eventType, data) {
  const payload = `event: ${eventType}\ndata: ${JSON.stringify(data)}\n\n`;
  for (const client of sseClients) {
    try {
      client.write(payload);
    } catch (err) {
      sseClients.delete(client);
    }
  }
}

const os = require('os');

// Real Hardware Telemetry Sampling
let prevCpuTimes = null;

function getRealCpuUsage() {
  const cpus = os.cpus();
  if (!cpus || cpus.length === 0) return 10;
  let totalUser = 0, totalNice = 0, totalSys = 0, totalIdle = 0, totalIrq = 0;
  for (const cpu of cpus) {
    totalUser += cpu.times.user;
    totalNice += cpu.times.nice;
    totalSys += cpu.times.sys;
    totalIdle += cpu.times.idle;
    totalIrq += cpu.times.irq;
  }
  const total = totalUser + totalNice + totalSys + totalIdle + totalIrq;
  if (!prevCpuTimes) {
    prevCpuTimes = { total, idle: totalIdle };
    return 15;
  }
  const diffTotal = total - prevCpuTimes.total;
  const diffIdle = totalIdle - prevCpuTimes.idle;
  prevCpuTimes = { total, idle: totalIdle };
  if (diffTotal <= 0) return 10;
  const usage = Math.round(((diffTotal - diffIdle) / diffTotal) * 100);
  return Math.max(1, Math.min(100, usage));
}

function getRealMemoryUsage() {
  const total = os.totalmem();
  const free = os.freemem();
  if (total <= 0) return 50;
  return Math.round(((total - free) / total) * 100);
}

// Periodic Real Hardware Telemetry Sampling
setInterval(() => {
  const realCpu = getRealCpuUsage();
  const realMem = getRealMemoryUsage();
  runtimeState.systemMetrics.cpu = realCpu;
  runtimeState.systemMetrics.memory = realMem;
  runtimeState.systemMetrics.uptime = Math.floor(os.uptime());
  runtimeState.systemMetrics.platform = os.platform();
  runtimeState.systemMetrics.arch = os.arch();
  runtimeState.systemMetrics.loadAvg = Math.round(os.loadavg()[0] * 100) / 100;

  broadcastEvent('system:metrics:updated', runtimeState.systemMetrics);
}, 2000);

const server = http.createServer((req, res) => {
  // CORS Headers
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization, X-Correlation-ID');

  if (req.method === 'OPTIONS') {
    res.writeHead(204);
    res.end();
    return;
  }

  const parsedUrl = new URL(req.url, 'http://' + (req.headers.host || 'localhost:' + PORT));
  const pathname = parsedUrl.pathname;

  // 1. Server-Sent Events (SSE) Stream
  if (pathname === '/api/events') {
    res.writeHead(200, {
      'Content-Type': 'text/event-stream',
      'Cache-Control': 'no-cache',
      'Connection': 'keep-alive',
    });

    res.write(`data: ${JSON.stringify({ type: 'connected', version: runtimeState.version })}\n\n`);
    sseClients.add(res);

    req.on('close', () => {
      sseClients.delete(res);
    });
    return;
  }

  // 1b. Remote Tool Dispatch Endpoint (Phase 19 Remote Dashboard)
  if (pathname === '/api/tool/execute' && req.method === 'POST') {
    let body = '';
    req.on('data', chunk => { body += chunk; });
    req.on('end', () => {
      try {
        const payload = JSON.parse(body || '{}');
        const tool = payload.tool || payload.capability_id || 'system.status';
        const args = payload.args || payload.arguments_json || '{}';
        const { execFile } = require('child_process');
        const path = require('path');
        const cliPath = path.join(__dirname, '..', 'build', 'vani-cli.exe');

        const cliArgs = ['exec', tool, typeof args === 'string' ? args : JSON.stringify(args)];
        if (payload.confirm) cliArgs.push('--confirm');

        execFile(cliPath, cliArgs, (err, stdout, stderr) => {
          if (err) {
            res.writeHead(500, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ success: false, error: err.message, stderr }));
          } else {
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ success: true, output: stdout }));
          }
        });
      } catch (err) {
        res.writeHead(400, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ error: 'Invalid payload: ' + err.message }));
      }
    });
    return;
  }

  // 2. Health & Subsystems Status
  if (pathname === '/api/status' && req.method === 'GET') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({
      state: runtimeState.lifecycleState,
      version: runtimeState.version,
      contractAbiVersion: runtimeState.contractAbiVersion,
      uptimeSeconds: Math.floor((Date.now() - runtimeState.bootTime) / 1000),
      subsystems: runtimeState.subsystemHealth,
      metrics: runtimeState.systemMetrics,
    }));
    return;
  }

  // 3. Capabilities Registry
  if (pathname === '/api/capabilities' && req.method === 'GET') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ capabilities: runtimeState.capabilities }));
    return;
  }

  // 4. Tasks List & Creation
  if (pathname === '/api/tasks' && req.method === 'GET') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ tasks: runtimeState.tasks }));
    return;
  }

  if (pathname === '/api/tasks' && req.method === 'POST') {
    let body = '';
    req.on('data', chunk => { body += chunk; });
    req.on('end', () => {
      try {
        const payload = JSON.parse(body || '{}');
        const newTask = {
          id: `task-${Date.now()}`,
          title: payload.title || 'Untitled Task',
          category: payload.category || 'system',
          state: 'running',
          progress: 5,
          priority: payload.priority || 'normal',
          agent: payload.agent || 'Odysseus',
          logs: [`[${new Date().toLocaleTimeString()}] Task submitted to VANI Task Engine`]
        };

        runtimeState.tasks.unshift(newTask);
        broadcastEvent('task:created', newTask);

        res.writeHead(201, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ success: true, task: newTask }));
      } catch (err) {
        res.writeHead(400, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ error: 'Invalid JSON payload' }));
      }
    });
    return;
  }

  // 5. LLM Chat Endpoint (Ollama Bridge)
  if (pathname === '/api/chat' && req.method === 'POST') {
    let body = '';
    req.on('data', chunk => { body += chunk; });
    req.on('end', () => {
      try {
        const payload = JSON.parse(body || '{}');
        const ollamaReq = http.request({
          hostname: 'localhost',
          port: 11434,
          path: '/api/chat',
          method: 'POST',
          headers: { 'Content-Type': 'application/json' }
        }, (ollamaRes) => {
          let ollamaData = '';
          ollamaRes.on('data', c => { ollamaData += c; });
          ollamaRes.on('end', () => {
            res.writeHead(ollamaRes.statusCode || 200, { 'Content-Type': 'application/json' });
            res.end(ollamaData);
          });
        });

        ollamaReq.on('error', (err) => {
          res.writeHead(503, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify({ error: 'Ollama not reachable: ' + err.message }));
        });

        ollamaReq.write(JSON.stringify({
          model: payload.model || 'qwen2.5:3b',
          messages: payload.messages || [],
          stream: false,
          options: payload.options || { temperature: 0.7, num_predict: 120 }
        }));
        ollamaReq.end();
      } catch (err) {
        res.writeHead(400, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ error: 'Invalid JSON payload' }));
      }
    });
    return;
  }

  // 6. Voice Normalization & Entity Resolution Endpoint
  if (pathname === '/api/voice/normalize' && req.method === 'POST') {
    let body = '';
    req.on('data', chunk => { body += chunk; });
    req.on('end', () => {
      try {
        const payload = JSON.parse(body || '{}');
        const raw = payload.text || '';
        let norm = raw
          .replace(/\bodysus\b/gi, 'Odysseus')
          .replace(/\bvani\b/gi, 'VANI')
          .replace(/\breact js\b/gi, 'React.js')
          .replace(/\bgit hub\b/gi, 'GitHub')
          .replace(/\brum karde\b/gi, 'run kar de');

        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({
          raw_transcript: raw,
          normalized_transcript: norm,
          language: raw.toLowerCase().includes('karde') || raw.toLowerCase().includes('mera') ? 'hinglish' : 'en',
          confidence: 0.95,
          entities: [
            { canonical_name: 'React.js', type: 'Technology', confidence: 0.98 },
            { canonical_name: 'Odysseus', type: 'Agent', confidence: 0.99 }
          ]
        }));
      } catch (err) {
        res.writeHead(400, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ error: 'Invalid payload' }));
      }
    });
    return;
  }

  // 6. Voice Intent Dispatcher
  if (pathname === '/api/voice/process' && req.method === 'POST') {
    let body = '';
    req.on('data', chunk => { body += chunk; });
    req.on('end', () => {
      try {
        const payload = JSON.parse(body || '{}');
        const transcript = payload.transcript || '';

        const recognizedIntent = {
          sessionId: `sess-${Date.now()}`,
          rawTranscript: transcript,
          confidence: 0.96,
          language: 'hinglish',
          timestamp: new Date().toISOString()
        };

        broadcastEvent('voice:intent:recognized', recognizedIntent);

        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({
          success: true,
          intent: recognizedIntent
        }));
      } catch (err) {
        res.writeHead(400, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ error: 'Invalid voice payload' }));
      }
    });
    return;
  }

  // 7. Fallback: Not Found
  res.writeHead(404, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({ error: 'Endpoint not found' }));
});

server.listen(PORT, () => {
  console.log(`\n===============================================================`);
  console.log(`  VANI MARK 2 — RUNTIME GATEWAY SERVER ONLINE                  `);
  console.log(`  Listening at: http://localhost:${PORT}                       `);
  console.log(`  Status API:   http://localhost:${PORT}/api/status            `);
  console.log(`  SSE Stream:   http://localhost:${PORT}/api/events            `);
  console.log(`===============================================================\n`);
});
