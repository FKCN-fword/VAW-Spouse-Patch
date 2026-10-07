// ============================================================================
//  VAWSpouseSwitcher — SKSE plugin (Skyrim SE 1.5.97)
//
//  Purpose
//  -------
//  Vittoria Vici and Asgeir Snow-Shod are linked by the vanilla relationship
//  record (RELA) "VittoriaAsgeir" (0x0001F729).  A relationship has TWO
//  independent layers:
//
//    1. Relationship Rank   (-4 .. +4)  -> settable at runtime via Papyrus
//    2. Association Type    (ASTP form) -> NOT settable via vanilla Papyrus
//
//  This DLL exposes the missing piece: it can change a relationship's
//  Association Type (and rank) at runtime.  The mod's quest script calls it
//  at the moment the wedding ends, so that:
//
//      before the wedding : Courting
//      after  the wedding : Spouse        <- this plugin flips it here
//
//  Because the modified values differ from the game-start values, Skyrim's
//  ChangeForm system records them in the save.  Loading a pre-wedding save
//  therefore restores Courting automatically, and loading a post-wedding
//  save keeps Spouse.
//
//  Papyrus interface (script: VAWSpouseSwitcher.psc)
//  -------------------------------------------------
//    Bool SetAssociationType(Actor a, Actor b, Int astpFormID)          Global Native
//    Int  GetAssociationType(Actor a, Actor b)                          Global Native
//    Bool SetRelationshipRankAndAssoc(Actor a, Actor b, Int rank, Int astpFormID) Global Native
// ============================================================================

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <REL/Relocation.h>

#include <spdlog/sinks/basic_file_sink.h>

#include <cstdint>

namespace
{
    // ------------------------------------------------------------------
    //  Logging
    // ------------------------------------------------------------------
    void InitializeLog()
    {
        auto path = SKSE::log::log_directory();
        if (!path) {
            return;
        }
        *path /= "VAWSpouseSwitcher.log";
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);
        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%H:%M:%S] [%l] %v");
    }

    // ------------------------------------------------------------------
    //  Helpers
    // ------------------------------------------------------------------
    // Base (NPC_) form of an actor; null on failure.
    RE::TESNPC* GetNPC(RE::Actor* a_actor)
    {
        return a_actor ? a_actor->GetActorBase() : nullptr;
    }

    // Look up an AssociationType form by FormID.
    //   TESForm::LookupByID<T> searches the global form map and is therefore
    //   independent of load order (works for Skyrim.esm and mod forms alike).
    RE::BGSAssociationType* LookupASTP(RE::FormID a_formID)
    {
        return RE::TESForm::LookupByID<RE::BGSAssociationType>(a_formID);
    }

    // Find the RELA record between two actors, trying both argument orders.
    RE::BGSRelationship* FindRelationship(RE::TESNPC* a_npcA, RE::TESNPC* a_npcB)
    {
        auto* rela = RE::BGSRelationship::GetRelationship(a_npcA, a_npcB);
        if (!rela) {
            rela = RE::BGSRelationship::GetRelationship(a_npcB, a_npcA);
        }
        return rela;
    }
}

// ======================================================================
//  Papyrus native functions
// ======================================================================
namespace Papyrus
{
    bool SetAssociationType(RE::StaticFunctionTag*,
                            RE::Actor* a_actorA,
                            RE::Actor* a_actorB,
                            std::int32_t a_astpFormID)
    {
        auto* npcA = GetNPC(a_actorA);
        auto* npcB = GetNPC(a_actorB);
        if (!npcA || !npcB) {
            SKSE::log::error("SetAssociationType: invalid actor(s)");
            return false;
        }

        auto* rela = FindRelationship(npcA, npcB);
        if (!rela) {
            SKSE::log::error("SetAssociationType: no RELA record found between the two actors");
            return false;
        }

        if (a_astpFormID == 0) {
            rela->assocType = nullptr;  // clear
            SKSE::log::info("SetAssociationType: cleared association type");
            return true;
        }

        auto* astp = LookupASTP(static_cast<RE::FormID>(a_astpFormID));
        if (!astp) {
            SKSE::log::error("SetAssociationType: ASTP form {:08X} not found",
                             static_cast<std::uint32_t>(a_astpFormID));
            return false;
        }

        rela->assocType = astp;
        SKSE::log::info("SetAssociationType: set assocType to {:08X}",
                        static_cast<std::uint32_t>(a_astpFormID));
        return true;
    }

    std::int32_t GetAssociationType(RE::StaticFunctionTag*,
                                    RE::Actor* a_actorA,
                                    RE::Actor* a_actorB)
    {
        auto* npcA = GetNPC(a_actorA);
        auto* npcB = GetNPC(a_actorB);
        if (!npcA || !npcB) {
            return 0;
        }

        auto* rela = FindRelationship(npcA, npcB);
        if (!rela || !rela->assocType) {
            return 0;
        }
        return static_cast<std::int32_t>(rela->assocType->GetFormID());
    }

    bool SetRelationshipRankAndAssoc(RE::StaticFunctionTag*,
                                     RE::Actor* a_actorA,
                                     RE::Actor* a_actorB,
                                     std::int32_t a_rank,
                                     std::int32_t a_astpFormID)
    {
        auto* npcA = GetNPC(a_actorA);
        auto* npcB = GetNPC(a_actorB);
        if (!npcA || !npcB) {
            SKSE::log::error("SetRelationshipRankAndAssoc: invalid actor(s)");
            return false;
        }

        auto* rela = FindRelationship(npcA, npcB);
        if (!rela) {
            SKSE::log::error("SetRelationshipRankAndAssoc: no RELA record found");
            return false;
        }

        // rank: GetRelationshipRank scale (-4..4) -> stored enum (0..8)
        //   rank 4 (Lover)        -> raw 0
        //   rank 0 (Acquaintance) -> raw 4
        const auto raw = static_cast<std::uint8_t>(4 - a_rank);
        if (raw <= 8) {
            rela->level = static_cast<RE::BGSRelationship::RELATIONSHIP_LEVEL>(raw);
        }

        if (a_astpFormID == 0) {
            rela->assocType = nullptr;
        } else if (auto* astp = LookupASTP(static_cast<RE::FormID>(a_astpFormID))) {
            rela->assocType = astp;
        } else {
            SKSE::log::error("SetRelationshipRankAndAssoc: ASTP {:08X} not found",
                             static_cast<std::uint32_t>(a_astpFormID));
            return false;
        }

        SKSE::log::info("SetRelationshipRankAndAssoc: rank={} assoc={:08X}", a_rank,
                        static_cast<std::uint32_t>(a_astpFormID));
        return true;
    }

    bool RegisterFuncs(RE::BSScript::IVirtualMachine* a_vm)
    {
        if (!a_vm) {
            return false;
        }

        a_vm->RegisterFunction("SetAssociationType", "VAWSpouseSwitcher", SetAssociationType, false);
        a_vm->RegisterFunction("GetAssociationType", "VAWSpouseSwitcher", GetAssociationType, false);
        a_vm->RegisterFunction("SetRelationshipRankAndAssoc", "VAWSpouseSwitcher", SetRelationshipRankAndAssoc, false);

        SKSE::log::info("VAWSpouseSwitcher: registered 3 papyrus functions");
        return true;
    }
}

// ======================================================================
//  Plugin entry point
// ======================================================================
SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    InitializeLog();

    // REL::Version has no .string() member; std::to_string(REL::Version) is provided.
    SKSE::log::info("VAWSpouseSwitcher loaded (game version {})", std::to_string(a_skse->RuntimeVersion()));

    const auto papyrus = SKSE::GetPapyrusInterface();
    if (!papyrus) {
        SKSE::log::error("Papyrus interface not available");
        return false;
    }
    if (!papyrus->Register(Papyrus::RegisterFuncs)) {
        SKSE::log::error("Failed to register papyrus functions");
        return false;
    }

    return true;
}
