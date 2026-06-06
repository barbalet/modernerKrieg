# AI Policies

This directory is reserved for AI controller policies.

AI code should consume core snapshots and submit normal core order structs. Human-vs-AI, AI-vs-AI, replay playback, and tests should all use the same order-validation and fixed-step simulation path.

No AI policy is implemented yet. The first milestone only establishes the folder boundary and headless runner needed by later AI-vs-AI cycles.
