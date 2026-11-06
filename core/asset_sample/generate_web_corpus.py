#!/usr/bin/env python3
"""
Project Axiom — Web Development Corpus Generator
File: core/asset_sample/generate_web_corpus.py

Generates a comprehensive ChatML training corpus covering:
1. Markup & Accessibility: Semantic HTML5, ARIA roles, forms, validation.
2. Styling: Vanilla CSS (Flexbox, Grid), Tailwind CSS, Bootstrap 5.
3. JavaScript Core & Runtime: Closures, Event Loop, Promises, Node.js streams.
4. Modern Web Frameworks: React, React Native, Vue 3, Angular.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def build_html_corpus() -> list[dict[str, str]]:
    """Generates examples on HTML5 semantics, accessibility, and forms."""
    examples_list: list[dict[str, str]] = []

    examples_list.append({
        "prompt": "Explain semantic HTML5 document architecture and contrast it with generic division layouts.",
        "response": """Semantic HTML5 introduces structural elements that convey precise programmatic meaning to user agents, assistive technologies, and search engines:

### Semantic Elements:
- `<header>`: Represents introductory content or navigation links for the document or section.
- `<nav>`: Encapsulates major navigational links.
- `<main>`: Encloses the central, non-repeating content unique to the document.
- `<article>`: Encapsulates an independent, self-contained composition (e.g., blog post, comment, card).
- `<section>`: Groups thematically related content, typically accompanied by an explicit heading.
- `<aside>`: Contains tangential or complementary information (e.g., sidebars, callouts).
- `<footer>`: Represents the footer for its nearest sectioning ancestor, typically holding copyright or legal links.

### Architectural Contrast:
```html
<!-- Anti-pattern: Non-semantic division soup -->
<div class="header">
  <div class="navigation">...</div>
</div>
<div class="content-wrapper">
  <div class="post">...</div>
</div>

<!-- Recommended: Semantic HTML5 Tree -->
<header>
  <nav aria-label="Main Navigation">
    <ul>
      <li><a href="/dashboard">Dashboard</a></li>
    </ul>
  </nav>
</header>
<main>
  <article>
    <h1>Low-Latency Engine Design</h1>
    <p>Core architectural breakdown...</p>
  </article>
</main>
<footer>
  <p>&copy; 2026 Project Axiom</p>
</footer>
```

### Benefits:
1. **Accessibility (a11y):** Screen readers expose landmarks (`main`, `nav`, `banner`), allowing visually impaired users to jump directly across document landmarks.
2. **Parsing Efficiency:** Headless parsers extract content hierarchies reliably without relying on volatile CSS class selectors."""
    })

    examples_list.append({
        "prompt": "How do you implement an accessible, accessible modal dialog using native HTML5 and WAI-ARIA?",
        "response": """Native HTML5 provides the `<dialog>` element, which handles top-layer rendering, backdrop styling, and focus traps when opened via `showModal()`.

### Accessible Implementation Pattern:
```html
<button id="open-button" type="button" aria-haspopup="dialog">
  Open Preferences
</button>

<dialog id="preference-dialog" aria-labelledby="dialog-title" aria-describedby="dialog-description">
  <form method="dialog">
    <h2 id="dialog-title">System Preferences</h2>
    <p id="dialog-description">Configure your local hardware acceleration preferences.</p>
    
    <label for="backend-selection">Inference Engine:</label>
    <select id="backend-selection" name="backend" required>
      <option value="cuda">CUDA 12.8 (Accelerated)</option>
      <option value="cpu">Host CPU Multi-thread</option>
    </select>

    <div class="actions">
      <button type="submit" value="confirm">Save Changes</button>
      <button type="submit" value="cancel" formnovalidate>Cancel</button>
    </div>
  </form>
</dialog>
```

### Script Controller:
```javascript
const openButton = document.getElementById('open-button');
const dialogElement = document.getElementById('preference-dialog');

openButton.addEventListener('click', () => {
  // showModal sets up an accessible modal layer and focus trap
  dialogElement.showModal();
});

dialogElement.addEventListener('close', () => {
  console.log('Dialog return value:', dialogElement.returnValue);
  openButton.focus(); // Deterministically restore focus
});
```

### Critical Accessibility Features:
1. **Top-Layer Stacking:** `<dialog>` automatically isolates z-index conflicts without high numeric z-index hacks.
2. **Keyboard Escape Handling:** Pressing the `Escape` key automatically cancels the dialog and fires the `close` event.
3. **Inert Background:** The underlying document content is marked inert, preventing background interaction."""
    })

    return examples_list


def build_styling_corpus() -> list[dict[str, str]]:
    """Generates examples covering Vanilla CSS, Tailwind CSS, and Bootstrap."""
    examples_list: list[dict[str, str]] = []

    examples_list.append({
        "prompt": "Explain the CSS Box Model, specificity hierarchy, and compare Flexbox with CSS Grid.",
        "response": """Styling engines compute layout dimensions and cascade priorities through fundamental geometric and algorithmic models:

### 1. The CSS Box Model:
Every rendered element is an enclosed rectangular box comprising:
- **Content:** The actual text, image, or media.
- **Padding:** Transparent spacing surrounding the content within the border boundary.
- **Border:** Stroke wrapping the padding area.
- **Margin:** Transparent buffer separating the element from neighboring elements.
- **`box-sizing: border-box`:** Forces `width` and `height` calculations to include padding and border, preventing layout breakage:
  $$\\text{Total Width} = \\text{Specified Width} + \\text{Margin Left} + \\text{Margin Right}$$

### 2. Specificity Hierarchy (Cascade Priority):
Specificity determines which rule overrides when multiple selectors target the same element:
1. `!important` declaration (highest priority; overrides standard cascade).
2. Inline styles (`style="..."`): (1, 0, 0, 0).
3. IDs (`#header`): (0, 1, 0, 0).
4. Classes, Attributes, and Pseudo-classes (`.card`, `[type="text"]`, `:hover`): (0, 0, 1, 0).
5. Type selectors and Pseudo-elements (`div`, `p`, `::before`): (0, 0, 0, 1).

### 3. Flexbox versus CSS Grid:
| Feature | Flexbox (One-Dimensional) | CSS Grid (Two-Dimensional) |
| :--- | :--- | :--- |
| **Primary Axis** | Single axis at a time (row OR column). | Dual axes simultaneously (rows AND columns). |
| **Ideal Use Case** | Navbars, button clusters, form input rows. | Full page layouts, dashboard cards, image galleries. |
| **Content vs Layout** | Content-driven: elements define their size. | Layout-driven: grid tracks define element size. |

```css
/* Responsive Grid Pattern without Media Queries */
.dashboard-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 1.5rem;
}
```"""
    })

    examples_list.append({
        "prompt": "Compare Tailwind CSS utility-first methodology with Bootstrap 5 component-based architecture.",
        "response": """Tailwind CSS and Bootstrap represent two distinct philosophies for authoring user interfaces:

### 1. Tailwind CSS (Utility-First Philosophy):
- **Mechanism:** Provides fine-grained, atomic utility classes directly in markup.
- **Strengths:** Zero CSS file bloat (unused classes purged at build time), complete design flexibility, eliminates context-switching between HTML and CSS files.
- **Implementation:**
```html
<div class="max-w-md mx-auto p-6 bg-slate-900 border border-slate-800 rounded-xl shadow-lg flex items-center space-x-4">
  <div class="shrink-0">
    <div class="h-12 w-12 rounded-full bg-cyan-500/20 text-cyan-400 flex items-center justify-center font-bold">
      AX
    </div>
  </div>
  <div>
    <h3 class="text-lg font-semibold text-white">Project Axiom</h3>
    <p class="text-sm text-slate-400">High-performance C++23 & CUDA Inference Engine</p>
  </div>
</div>
```

### 2. Bootstrap 5 (Component-Based Philosophy):
- **Mechanism:** Provides predefined, opinionated components with standard styling and JavaScript behaviors.
- **Strengths:** Rapid prototyping, consistent out-of-the-box accessible controls, simple 12-column flexbox grid.
- **Implementation:**
```html
<div class="card bg-dark text-light border-secondary mx-auto" style="max-width: 450px;">
  <div class="card-body d-flex align-items-center">
    <div class="avatar bg-primary text-white rounded-circle p-3 me-3 fw-bold">
      AX
    </div>
    <div>
      <h5 class="card-title mb-1">Project Axiom</h5>
      <p class="card-text text-muted mb-0">High-performance C++23 & CUDA Inference Engine</p>
    </div>
  </div>
</div>
```

### Selection Criteria:
- Choose **Tailwind** when building custom bespoke design systems requiring strict design tokens and zero style overrides.
- Choose **Bootstrap** when prioritizing rapid internal tooling deployment and standard off-the-shelf component libraries."""
    })

    return examples_list


def build_javascript_corpus() -> list[dict[str, str]]:
    """Generates examples covering Vanilla JavaScript, the Event Loop, and Node.js."""
    examples_list: list[dict[str, str]] = []

    examples_list.append({
        "prompt": "Explain JavaScript execution contexts, closures, and the event loop microtask/macrotask queuing model.",
        "response": """JavaScript executes on a single-threaded event loop backed by the V8 engine and Libuv:

### 1. Execution Context & Closures:
- **Execution Context:** Comprises the Variable Environment, Lexical Environment, and `this` binding value.
- **Closure:** A function retains a lexical reference to its outer scope even when executed outside that outer scope:
```javascript
function createRateLimiter(maximumTokens, refillIntervalMilliseconds) {
  let tokens = maximumTokens; // Enclosed lexical state

  return function attemptAcquire() {
    if (tokens > 0) {
      tokens -= 1;
      return true;
    }
    return false;
  };
}
```

### 2. The Event Loop Architecture:
The execution model consists of three core structures:
1. **Call Stack:** Executes synchronous frames in LIFO order.
2. **Microtask Queue:** Processes jobs that must run immediately after the current synchronous script without yielding to the browser rendering engine:
   - `Promise.then()`, `Promise.catch()`, `Promise.finally()`
   - `queueMicrotask()`
   - `MutationObserver`
3. **Macrotask (Task) Queue:** Processes discrete asynchronous tasks scheduled by the host environment:
   - `setTimeout()`, `setInterval()`
   - DOM events (clicks, scrolls)
   - I/O callbacks, `setImmediate()` (Node.js)

### Order of Operations:
1. Execute the entire synchronous call stack until empty.
2. Flush **all** tasks in the microtask queue sequentially. If a microtask enqueues another microtask, it is executed within the same cycle.
3. Perform UI render recalculation (browser only).
4. Dequeue and execute exactly **one** macrotask from the task queue.
5. Repeat cycle."""
    })

    examples_list.append({
        "prompt": "How do you implement high-throughput, backpressure-safe streaming in Node.js using stream.pipeline?",
        "response": """In Node.js, streaming large binary datasets without backpressure management causes unbounded memory buffering in RAM, triggering process Out-Of-Memory (OOM) crashes.

### The Backpressure Solution via `stream.pipeline`:
`stream.pipeline` automatically listens to `'drain'` events, pauses the readable stream when the downstream writable buffer saturates, and safely cleans up file descriptors upon errors.

```javascript
import { pipeline } from 'node:stream/promises';
import { createReadStream, createWriteStream } from 'node:fs';
import { createGzip } from 'node:zlib';
import { Transform } from 'node:stream';

// Custom Transform Stream for parsing line-delimited records
const recordTransformer = new Transform({
  objectMode: true,
  transform(chunk, encoding, callback) {
    try {
      const sanitized = chunk.toString().trim().toUpperCase();
      this.push(`${sanitized}\\n`);
      callback();
    } catch (error) {
      callback(error);
    }
  }
});

async function streamCompressArchive(sourcePath, destinationPath) {
  try {
    await pipeline(
      createReadStream(sourcePath, { highWaterMark: 64 * 1024 }), // 64 KB chunks
      recordTransformer,
      createGzip({ level: 6 }),
      createWriteStream(destinationPath)
    );
    console.log('[Node Stream] Compression pipeline completed successfully.');
  } catch (error) {
    console.error('[Node Stream] Pipeline failed with error:', error);
    throw error;
  }
}
```

### Key Mechanisms:
- **`highWaterMark`:** Defines internal buffer threshold in bytes (default 16 KB for standard streams, 64 KB for file streams).
- **Automatic Cleanup:** Guarantees all underlying operating system handles are closed whether the pipeline resolves or throws."""
    })

    return examples_list


def build_frameworks_corpus() -> list[dict[str, str]]:
    """Generates examples covering React, React Native, Vue 3, and Angular."""
    examples_list: list[dict[str, str]] = []

    examples_list.append({
        "prompt": "Explain React Fiber reconciliation, hook rules, and state memoization with useMemo and useCallback.",
        "response": """React employs a cooperative scheduling reconciliation architecture termed React Fiber:

### 1. Fiber Reconciliation Engine:
- **Work Units:** Every React element is converted into a Fiber node (a lightweight JavaScript object representing a component instance and its DOM state).
- **Dual-Buffering:** React maintains two Fiber trees:
  - `current`: The tree currently rendered on screen.
  - `workInProgress`: The tree being constructed asynchronously in memory.
- **Commit Phase:** Once work on `workInProgress` is complete, React swaps the root pointer in a single synchronous commit to avoid visual tearing.

### 2. State Memoization (`useMemo` vs `useCallback`):
- `useMemo`: Caches the *computed result* of an expensive calculation between renders:
  ```typescript
  const filteredTokens = useMemo(() => {
    return tokenList.filter((item) => item.frequency >= minimumFrequency);
  }, [tokenList, minimumFrequency]);
  ```
- `useCallback`: Caches a *function instance* definition between renders to maintain referential identity for memoized child components (`React.memo`):
  ```typescript
  const handleSelectToken = useCallback((tokenId: string) => {
    dispatch({ type: 'SELECT_TOKEN', payload: tokenId });
  }, [dispatch]);
  ```

### 3. Rules of Hooks:
1. **Call at the Top Level Only:** Never invoke hooks inside loops, conditions, or nested functions to ensure hook call order remains deterministic across render iterations.
2. **Call from React Functions Only:** Only invoke hooks from functional components or custom hooks."""
    })

    examples_list.append({
        "prompt": "Compare Vue 3 Composition API with Angular standalone components and dependency injection.",
        "response": """Vue 3 and Angular offer modern, declarative component models with distinct approaches to reactivity and dependency resolution:

### 1. Vue 3 Composition API (Proxy-Based Reactivity):
Vue 3 uses ES6 `Proxy` instances to intercept property reads (dependency tracking via `track()`) and writes (triggering component re-renders via `trigger()`):

```vue
<script setup lang="ts">
import { ref, computed, onMounted } from 'vue';

const counter = ref<number>(0);
const doubleCounter = computed(() => counter.value * 2);

function increment() {
  counter.value += 1;
}

onMounted(() => {
  console.log('[Vue Component] Mounted with initial counter:', counter.value);
});
</script>

<template>
  <button @click="increment">Count: {{ counter }} (Double: {{ doubleCounter }})</button>
</template>
```

### 2. Angular Standalone Components & Dependency Injection:
Angular features TypeScript decorators, class-based architecture, and hierarchical dependency injection without requiring intermediary `NgModule` definitions:

```typescript
import { Component, inject, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { TelemetryService } from './telemetry.service';

@Component({
  selector: 'axiom-telemetry-panel',
  standalone: true,
  imports: [CommonModule],
  template: `
    <div class="telemetry">
      <h3>GPU Memory: {{ memoryUsage() }} MB</h3>
      <button (click)="refreshMetrics()">Query Sensor</button>
    </div>
  `
})
export class TelemetryPanelComponent {
  private readonly telemetryService = inject(TelemetryService);
  readonly memoryUsage = signal<number>(0);

  async refreshMetrics(): Promise<void> {
    const usage = await this.telemetryService.queryAllocatedMegabytes();
    this.memoryUsage.set(usage);
  }
}
```

### Architectural Distinctions:
- **Reactivity:** Vue 3 uses automatic deep Proxy interception; Angular utilizes Signals (`signal()`, `computed()`) for fine-grained dependency tracking without Zone.js overhead.
- **Service Lifetime:** Angular provides hierarchical injector trees (`providedIn: 'root'`, component injectors); Vue utilizes the `provide`/`inject` API or Pinia stores."""
    })

    examples_list.append({
        "prompt": "Explain the architecture of React Native: Bridge, JavaScript Interface (JSI), and Fabric renderer.",
        "response": """React Native bridges web technology and native mobile operating systems (iOS and Android):

### 1. Legacy Architecture (The Asynchronous Bridge):
- **Three Threads:**
  1. JavaScript Thread (executes bundle code).
  2. Shadow Thread (calculates Yoga layout dimensions).
  3. Native UI Thread (renders platform views).
- **Bottleneck:** All communication traveled as serialized JSON strings asynchronously across a single C++ bridge. High-frequency interactions (e.g., gesture scrolling) experienced dropped frames due to serialization latency.

### 2. Modern Architecture (The New Architecture):
- **JavaScript Interface (JSI):** Replaces JSON serialization with direct C++ host object bindings. JavaScript can invoke native C++ methods synchronously in zero-copy memory.
- **Fabric Renderer:** The new rendering engine that renders UI trees directly through JSI, supporting concurrent React features on mobile devices.
- **TurboModules:** Native modules are loaded lazily on demand via JSI rather than all initializing eagerly at app startup, drastically improving application cold-start time.

```typescript
import React from 'react';
import { StyleSheet, Text, View, FlatList } from 'react-native';

interface BenchmarkItem {
  id: string;
  kernelName: string;
  latencyMicroseconds: number;
}

export const BenchmarkList: React.FC<{ data: BenchmarkItem[] }> = ({ data }) => {
  return (
    <View style={styles.container}>
      <FlatList
        data={data}
        keyExtractor={(item) => item.id}
        renderItem={({ item }) => (
          <View style={styles.row}>
            <Text style={styles.title}>{item.kernelName}</Text>
            <Text style={styles.metric}>{item.latencyMicroseconds} μs</Text>
          </View>
        )}
        initialNumToRender={10}
        maxToRenderPerBatch={10}
        windowSize={5}
      />
    </View>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#090d16' },
  row: { flexDirection: 'row', justifyContent: 'space-between', padding: 16 },
  title: { color: '#ffffff', fontSize: 16 },
  metric: { color: '#38bdf8', fontWeight: 'bold' },
});
```"""
    })

    return examples_list


def format_chatml(prompt: str, response: str) -> str:
    """Encodes a QA pair into Axiom ChatML standard conversational format."""
    return f"<|im_start|>user\n{prompt.strip()}<|im_end|>\n<|im_start|>assistant\n{response.strip()}<|im_end|>\n"


def generate_corpus(output_path: Path, repeat_multiplier: int = 1) -> int:
    """Assembles all web categories, formats in ChatML, and writes to disk."""
    examples_list: list[dict[str, str]] = []
    examples_list.extend(build_html_corpus())
    examples_list.extend(build_styling_corpus())
    examples_list.extend(build_javascript_corpus())
    examples_list.extend(build_frameworks_corpus())

    output_path.parent.mkdir(parents=True, exist_ok=True)

    total_characters = 0
    with open(output_path, "w", encoding="utf-8") as output_file:
        for _ in range(repeat_multiplier):
            for example in examples_list:
                chatml_block = format_chatml(example["prompt"], example["response"])
                output_file.write(chatml_block)
                total_characters += len(chatml_block)

    return total_characters


def main() -> None:
    """Entry point for the web development training corpus generator."""
    argument_parser = argparse.ArgumentParser(
        description="Project Axiom Web Development Corpus Generator"
    )
    argument_parser.add_argument(
        "--output", "-o",
        type=Path,
        default=Path("core/asset_sample/web_training_corpus.txt"),
        help="Path to output corpus text file."
    )
    argument_parser.add_argument(
        "--multiplier", "-m",
        type=int,
        default=3,
        help="Repeat count multiplier to scale corpus size."
    )
    parsed_arguments = argument_parser.parse_args()

    print("[Axiom Corpus] Generating Web Development corpus...")
    total_bytes = generate_corpus(parsed_arguments.output, parsed_arguments.multiplier)
    print(f"[Axiom Corpus] Successfully generated {parsed_arguments.output} "
          f"({total_bytes / 1024:.1f} KB, multiplier={parsed_arguments.multiplier}).")


if __name__ == "__main__":
    main()

