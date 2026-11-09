#pragma once

#include "character_stuff.h"
#include "helpers.h"
#include "uuid.h"
#include "world_time.h"

class World {
public:
    float timeScale = 1.0f; // At 1.0: 1 day = 2 seconds

    RecordStore<Character> characters;
    RecordStore<Pregnancy> pregnancies;
    RecordStore<FamilyRelation> family_relations;
    RecordStore<MarriageRelation> marriage_relations;
    RecordStore<CharaRelations> social_relastions;
    RecordStore<CharaOpinions> opinions;

    const json& names;
    const json& settings;

    WorldTime date{};

    World(const json& names, const json& settings)
        : names(names), settings(settings) {}

    // Call every frame, passing GetFrameTime().
    void Update(float deltaTime) {
        if (timeScale <= 0.0f)
            return;

        dayAccumulator += deltaTime * timeScale;

        while (dayAccumulator >= 2.0f) {
            dayAccumulator -= 2.0f;
            AdvanceDay();
        }
    }

private:
    float dayAccumulator = 0.0f;

    void AdvanceMonth() {
        date.month++;

        if (date.month > 12) {
            date.month = 1;
            AdvanceYear();
        }
    }

    void AdvanceYear() {
        date.year++;

        // Age characters before checking their death probability.
        AdvanceCharacterYear(characters);

        ProcessDeaths(
            characters,
            marriage_relations,
            date
        );
    }

    void AdvanceDay() {
        date.day++;

        if (date.day > 30) {
            date.day = 1;
            AdvanceMonth();
        }
        // Check pregnancies every simulated day.
        ProcessPregnancies(
            pregnancies,
            characters,
            family_relations,
            names,
            date
        );
    }
};