# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

KopiRPC (Kopi Remote Procedure Call) is a high-performance C++ RPC framework. Licensed under Apache 2.0.

The project is in the **theory-learning / pre-implementation phase** — no source code or build system exists yet. The planned technical direction (based on the reference article digested in `notes/01`):

- **ZooKeeper** — service registration & discovery (service name = persistent node, method name = ephemeral node, node data = IP:Port)
- **ProtoBuf** — serialization and service/interface definition (`.proto` → Stub class for caller, Service base class for callee)
- **Muduo** — Multi-Reactor multithreaded network library
- Later iterations (see 知识点14 in notes/01): client-side async calls, load balancing, retry, health check, circuit breaker, long connections / connection pool

Documentation, commit messages, and communication with the user are in Chinese.

## Repository Conventions

- **README.md is strictly user-facing**: what RPC is, why it's needed, application scenarios, roadmap, links to notes. Implementation details (call-flow diagrams, component internals, technology tables, code) must NOT go into README — they belong in the study notes. This is an explicit rule from the user.
- **notes/** holds numbered study notes (`01-`, `02-`, ...) generated from learning materials (articles, screenshots, videos) via the `course-notes-generator` skill: 10-15 anchored knowledge points in source order, FSM-style list format for flows, bilingual terminology `中文(English)`, exam-ready summary at the end, and source attribution (link + author) at the top.
- **Workflow the user expects**: before generating or restructuring content, first present a key-point outline (要点) for review and get approval, then execute. The user often edits generated files by hand afterward — preserve those manual edits when touching a file again.

## Current State

No build system yet. When code lands (e.g., CMake configuration, source layout, test framework), update this file with:

- Build, test, and lint commands (including how to run a single test)
- The actual architecture of the framework (transport layer, serialization, service registration/discovery, logging)
