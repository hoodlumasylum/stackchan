// SPDX-FileCopyrightText: 2026 Kenta IDA <fuga@fugafuga.org>
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "conversation/conversation_service.hpp"

namespace stackchan::conversation {

// Which OpenAI-Realtime-compatible server the client talks to.
//   OpenAi — wss://api.openai.com/v1/realtime (GA session shape)
//   Grok   — xAI Grok Voice Agent, wss://api.x.ai/v1/realtime. Same event
//            protocol; the session.update shape (voice / turn_detection at
//            the session top level) and a few events differ — see the .cpp.
enum class RealtimeDialect : std::uint8_t {
    OpenAi,
    Grok,
};

// ConversationService backed by the OpenAI Realtime API over a raw WebSocket
// (wss://api.openai.com/v1/realtime), or the xAI Grok Voice Agent which is
// protocol-compatible. Server-side VAD/STT/LLM/TTS — the whole pipeline runs
// in one session, so commit_audio() is a no-op here.
class OpenAiRealtimeClient final : public ConversationService {
public:
    // `api_key` is the OpenAI (or xAI, for RealtimeDialect::Grok) API key
    // used for the WebSocket upgrade (Authorization: Bearer ...).
    explicit OpenAiRealtimeClient(std::string api_key, RealtimeDialect dialect = RealtimeDialect::OpenAi);
    ~OpenAiRealtimeClient() override;

    OpenAiRealtimeClient(const OpenAiRealtimeClient&) = delete;
    OpenAiRealtimeClient& operator=(const OpenAiRealtimeClient&) = delete;

    void set_event_callback(EventCallback cb) override;
    tl::expected<void, ConversationError> start(const ConversationConfig& config) override;
    void stop() override;
    tl::expected<void, ConversationError> push_audio(std::span<const std::int16_t> pcm) override;
    tl::expected<void, ConversationError> commit_audio() override;
    tl::expected<void, ConversationError> submit_tool_result(std::string_view call_id,
                                                             std::string_view output_json) override;
    tl::expected<void, ConversationError> cancel_response() override;
    tl::expected<void, ConversationError> push_text(std::string_view text) override;
    ConversationState state() const override;
    std::uint32_t tx_evicted_chunks() const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace stackchan::conversation
