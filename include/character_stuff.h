#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include "helpers.h"
#include "uuid.h"
#include "world_time.h"

// ============================================================
// DATE HELPERS
// ============================================================

// Adds a number of months using the 30-day, 12-month calendar.
inline WorldTime AddMonths(WorldTime date, int months)
{
    long long totalMonths =
        static_cast<long long>(date.year) * 12 +
        (date.month - 1) + months;

    date.year = static_cast<int>(totalMonths / 12);
    date.month = static_cast<int>(totalMonths % 12) + 1;

    return date;
}

// Subtracts years while preserving the day and month.
inline WorldTime SubtractYears(WorldTime date, int years)
{
    date.year -= years;
    return date;
}

// ============================================================
//  1: STRUCTS AND CLASSES
// ============================================================

struct CharacterSkills {
    int martial = 0;
    int intrigue = 0;
    int stewardship = 0;
    int diplomacy = 0;
    int knowledge = 0;
};

struct Character {
    Uuid id{};

    std::string name;
    int age = 0;
    int gender = 0; // 0 = masculine, 1 = feminine

    int health = 100;
    int lifespan = 80;
    bool alive = true;

    WorldTime birthDate{};
    WorldTime deathDate{};

    CharacterSkills skills;

    Character(
        std::string name,
        int age,
        int gender,
        WorldTime birthDate
    )
        : name(std::move(name)),
          age(age),
          gender(gender),
          birthDate(birthDate) {}
};

struct CharaOpinions {
    Uuid id{};
    IdRef<Character> chara_a;
    IdRef<Character> chara_b;
    int opinion = 0;
};

enum class FamilyRel {
    Parent,
    Child,
    Sibling,
    Grandparent,
    Grandchild,
    Uncle,
    Aunt,
    Nephew,
    Niece,
    Cousin
};

enum class SocialRel {
    Acquaintance,
    Friend,
    Rival,
    Enemy
};

enum class MarriageStatus {
    Betrothed,
    Married,
    Divorced,
    Widowed,
    BetrothalBroken
};

struct CharaRelations {
    Uuid id{};
    IdRef<Character> chara_a;
    IdRef<Character> chara_b;
    SocialRel social_relation = SocialRel::Acquaintance;
};

// Opinions are directional: A's opinion of B != B's opinion of A.
using OpinionStore =
    RecordStore<CharaOpinions, PairMode::Directed>;

// Social relations are mutual: A and B are friends/rivals together.
using SocialRelationStore =
    RecordStore<CharaRelations, PairMode::Symmetric>;

struct FamilyRelation {
    Uuid id{};
    IdRef<Character> character;
    IdRef<Character> related_character;
    FamilyRel relation = FamilyRel::Child;
};

struct MarriageRelation {
    Uuid id{};

    IdRef<Character> chara_a;
    IdRef<Character> chara_b;

    MarriageStatus status = MarriageStatus::Betrothed;

    WorldTime startDate{};
    WorldTime endDate{};
};

enum class PregnancyStatus {
    Developing,
    Birthed,
    Miscarriage
};

struct Pregnancy {
    Uuid id{};

    IdRef<Character> mother;
    IdRef<Character> father;

    WorldTime conceptionDate{};

    int pregnancyDuration = 10;
    int children = 1;

    PregnancyStatus status = PregnancyStatus::Developing;
};

// ============================================================
//  2: RELATIONSHIP FUNCTIONS
// ============================================================

inline bool HasActivePartner(
    const RecordStore<MarriageRelation>& marriages,
    const RecordStore<Character>& characters,
    Uuid characterId)
{
    for (const auto& [id, marriage] : marriages) {
        if (marriage.status != MarriageStatus::Married &&
            marriage.status != MarriageStatus::Betrothed) {
            continue;
        }

        Uuid partnerId{};

        if (marriage.chara_a.id == characterId)
            partnerId = marriage.chara_b.id;
        else if (marriage.chara_b.id == characterId)
            partnerId = marriage.chara_a.id;
        else
            continue;

        const Character* partner = characters.Get(partnerId);

        if (partner && partner->alive)
            return true;
    }

    return false;
}

inline bool CanMarry(
    const json& settings,
    const RecordStore<MarriageRelation>& marriages,
    const RecordStore<Character>& characters,
    const Character& a,
    const Character& b)
{
    if (!a.alive || !b.alive || a.id == b.id)
        return false;

    const int marriageAge =
        settings.at("marriage-age").get<int>();

    if (a.age < marriageAge || b.age < marriageAge)
        return false;

    const bool monogamy =
        settings.at("monogamy").get<bool>();

    if (monogamy &&
        (HasActivePartner(marriages, characters, a.id) ||
         HasActivePartner(marriages, characters, b.id))) {
        return false;
    }

    return true;
}

inline bool CanBetroth(
    const json& settings,
    const RecordStore<MarriageRelation>& marriages,
    const RecordStore<Character>& characters,
    const Character& a,
    const Character& b)
{
    if (!a.alive || !b.alive || a.id == b.id)
        return false;

    const int betrothAge =
        settings.at("betrothed-age").get<int>();

    if (a.age < betrothAge || b.age < betrothAge)
        return false;

    const bool monogamy =
        settings.at("monogamy").get<bool>();

    if (monogamy &&
        (HasActivePartner(marriages, characters, a.id) ||
         HasActivePartner(marriages, characters, b.id))) {
        return false;
    }

    return true;
}

inline void CreateMarriage(
    RecordStore<MarriageRelation>& marriages,
    const Character& chara_a,
    const Character& chara_b,
    const WorldTime& date)
{
    if (!chara_a.alive || !chara_b.alive ||
        chara_a.id == chara_b.id) {
        return;
    }

    MarriageRelation rel;
    rel.chara_a = {chara_a.id};
    rel.chara_b = {chara_b.id};
    rel.status = MarriageStatus::Married;
    rel.startDate = date;

    marriages.Insert(rel);
}

inline void CreateBetrothal(
    RecordStore<MarriageRelation>& marriages,
    const Character& chara_a,
    const Character& chara_b,
    const WorldTime& date)
{
    if (!chara_a.alive || !chara_b.alive ||
        chara_a.id == chara_b.id) {
        return;
    }

    MarriageRelation rel;
    rel.chara_a = {chara_a.id};
    rel.chara_b = {chara_b.id};
    rel.status = MarriageStatus::Betrothed;
    rel.startDate = date;

    marriages.Insert(rel);
}

inline void ConsummateMarriage(
    RecordStore<MarriageRelation>& marriages,
    const Character& chara_a,
    const Character& chara_b,
    const WorldTime& date)
{
    if (!chara_a.alive || !chara_b.alive ||
        chara_a.id == chara_b.id) {
        return;
    }

    MarriageRelation* rel = marriages.Find(
        [&](const MarriageRelation& marriage) {
            const bool samePair =
                (marriage.chara_a.id == chara_a.id &&
                 marriage.chara_b.id == chara_b.id) ||
                (marriage.chara_a.id == chara_b.id &&
                 marriage.chara_b.id == chara_a.id);

            return samePair &&
                   marriage.status == MarriageStatus::Betrothed;
        });

    if (rel) {
        rel->status = MarriageStatus::Married;
        rel->startDate = date;
        rel->endDate = {};
    }
}

inline void BreakBetrothal(
    RecordStore<MarriageRelation>& marriages,
    const Character& chara_a,
    const Character& chara_b,
    const WorldTime& date)
{
    if (chara_a.id == chara_b.id)
        return;

    MarriageRelation* rel = marriages.Find(
        [&](const MarriageRelation& marriage) {
            const bool samePair =
                (marriage.chara_a.id == chara_a.id &&
                 marriage.chara_b.id == chara_b.id) ||
                (marriage.chara_a.id == chara_b.id &&
                 marriage.chara_b.id == chara_a.id);

            return samePair &&
                   marriage.status == MarriageStatus::Betrothed;
        });

    if (rel) {
        rel->status = MarriageStatus::BetrothalBroken;
        rel->endDate = date;
    }
}

inline void BreakMarriage(
    RecordStore<MarriageRelation>& marriages,
    const Character& chara_a,
    const Character& chara_b,
    const WorldTime& date)
{
    if (chara_a.id == chara_b.id)
        return;

    MarriageRelation* rel = marriages.Find(
        [&](const MarriageRelation& marriage) {
            const bool samePair =
                (marriage.chara_a.id == chara_a.id &&
                 marriage.chara_b.id == chara_b.id) ||
                (marriage.chara_a.id == chara_b.id &&
                 marriage.chara_b.id == chara_a.id);

            return samePair &&
                   marriage.status == MarriageStatus::Married;
        });

    if (rel) {
        rel->status = MarriageStatus::Divorced;
        rel->endDate = date;
    }
}

inline void ProcessWidowhood(
    RecordStore<MarriageRelation>& marriages,
    const Character& deceased,
    const WorldTime& date)
{
    for (auto& [id, marriage] : marriages) {
        if (marriage.status != MarriageStatus::Married)
            continue;

        const bool involvesDeceased =
            marriage.chara_a.id == deceased.id ||
            marriage.chara_b.id == deceased.id;

        if (involvesDeceased) {
            marriage.status = MarriageStatus::Widowed;
            marriage.endDate = date;
        }
    }
}

// ============================================================
//  3: CHARACTER CREATION
// ============================================================

inline Character CreateCharacter(
    RecordStore<Character>&characters,
    const json& names,
    const std::string& origin,
    int gender,
    const WorldTime& currentDate,
    int age = 0)
{
    if (gender != 0 && gender != 1)
        throw std::invalid_argument("Invalid gender");

    if (age < 0)
        throw std::invalid_argument("Invalid character age");

    const std::string genderKey =
        gender == 0 ? "masculine" : "feminine";

    const auto& namesList =
        names.at(origin).at(genderKey).at("names");

    if (!namesList.is_array() || namesList.empty())
        throw std::runtime_error("No names available");

    const int nameIndex = GetRandomIntValue(
        0, static_cast<int>(namesList.size()) - 1);

    const std::string name =
        namesList[nameIndex].get<std::string>();

    WorldTime birthDate = SubtractYears(currentDate, age);
    Character c{name, age, gender, birthDate};
    c.id = characters.Insert(c);
    return c;
}

// ============================================================
// CHILD CREATION
// ============================================================

// The father only needs to have been alive at conception.
// He does not need to be alive when the child is born.
inline bool WasAliveAtDate(
    const Character& character,
    const WorldTime& date)
{
    if (IsDateBefore(date, character.birthDate))
        return false;

    // A death date is treated as the date on which the character
    // is no longer alive.
    if (!character.alive &&
        !IsDateBefore(date, character.deathDate)) {
        return false;
    }

    return true;
}

inline Uuid CreateChildBetween(
    const json& names,
    RecordStore<FamilyRelation>& familyRelations,
    RecordStore<Character>& characters,
    const Character& mother,
    const Character& father,
    const WorldTime& conceptionDate,
    const WorldTime& birthDate)
{
    if (mother.id == father.id)
        return {};

    // Mother must be alive when the child is born.
    if (!mother.alive)
        return {};

    // Father must have been alive when conception occurred.
    if (!WasAliveAtDate(father, conceptionDate))
        return {};

    if (father.gender != 0 || mother.gender != 1)
        return {};

    if (IsDateBefore(birthDate, conceptionDate))
        return {};

    Character child = CreateCharacter(
        characters,
        names,
        "Game",
        GetRandomIntValue(0, 1),
        birthDate,
        0
    );

    // CreateCharacter already inserted the child and set its id.
    const Uuid childId = child.id;

    FamilyRelation relation;
    relation.character = {mother.id};
    relation.related_character = {childId};
    relation.relation = FamilyRel::Child;
    familyRelations.Insert(relation);

    relation.character = {father.id};
    familyRelations.Insert(relation);

    return childId;
}

// ============================================================
//  4: CHARACTER GENERATION AND SKILLS
// ============================================================

inline void CreateCharacters(
    RecordStore<Character>& characters,
    const json& names,
    const std::string& origin,
    int gender,
    int count,
    const WorldTime& currentDate,
    int min_age = 10,
    int max_age = 100)
{
    if (gender != 0 && gender != 1)
        throw std::invalid_argument("Invalid gender");

    if (count < 0 || min_age < 0 || max_age < min_age)
        throw std::invalid_argument(
            "Invalid character generation range");

    const std::string genderKey =
        gender == 0 ? "masculine" : "feminine";

    const auto& namesList =
        names.at(origin).at(genderKey).at("names");

    if (!namesList.is_array() || namesList.empty())
        throw std::runtime_error("No names available");

    const int nameCount =
        static_cast<int>(namesList.size());
    for (int i = 0; i < count; ++i) {
        const int nameIndex =
            GetRandomIntValue(0, nameCount - 1);

        const int age =
            GetRandomIntValue(min_age, max_age);

        const std::string name =
            namesList[nameIndex].get<std::string>();

        WorldTime birthDate =
            SubtractYears(currentDate, age);

        characters.Insert({
            name,
            age,
            gender,
            birthDate}
        );
    }
}

inline CharacterSkills GenerateSkills()
{
    CharacterSkills skills;

    skills.martial =
        GetRandomIntValueBiased(0, 3, 0.2f);

    skills.intrigue =
        GetRandomIntValueBiased(0, 3, 0.2f);

    skills.stewardship =
        GetRandomIntValueBiased(0, 3, 0.2f);

    skills.diplomacy =
        GetRandomIntValueBiased(0, 3, 0.2f);

    skills.knowledge =
        GetRandomIntValueBiased(0, 3, 0.2f);

    return skills;
}

// ============================================================
//  5: OPINIONS AND CHARACTER PROGRESSION
// ============================================================

// What `from` thinks of `to`. Strangers have no record yet, so
// they read as neutral (0) without anything being created.
inline int GetOpinion(
    const OpinionStore& opinions,
    Uuid from,
    Uuid to)
{
    const CharaOpinions* record = opinions.GetPair(from, to);
    return record ? record->opinion : 0;
}

// Changes `from`'s opinion of `to`, creating the record on first
// interaction. Only `from`'s side changes; `to`'s opinion of
// `from` is a separate record.
inline void ModifyOpinion(
    OpinionStore& opinions,
    Uuid from,
    Uuid to,
    int delta)
{
    CharaOpinions& record = opinions.GetOrCreate(from, to);
    record.opinion = std::clamp(record.opinion + delta, -100, 100);
}

// Acquaintance unless a record says otherwise. Order doesn't matter.
inline SocialRel GetSocialRelation(
    const SocialRelationStore& relations,
    Uuid a,
    Uuid b)
{
    const CharaRelations* record = relations.GetPair(a, b);
    return record ? record->social_relation : SocialRel::Acquaintance;
}

inline void SetSocialRelation(
    SocialRelationStore& relations,
    Uuid a,
    Uuid b,
    SocialRel relation)
{
    relations.GetOrCreate(a, b).social_relation = relation;
}

inline void AdvanceCharacterYear(
    RecordStore<Character>& characters)
{
    for (auto& [id, character] : characters) {
        if (character.alive)
            character.age += 1;
    }
}

inline float GetCharaDeathProbability(
    Character& character,
    float ageInfluence = 0.55f,
    float ageExponent = 2.0f,
    float healthOffset = 0.00001f)
{
    if (character.lifespan <= 0)
        return 1.0f;

    character.health =
        std::clamp(character.health, 0, 100);
    const float ageRatio =
        static_cast<float>(character.age) /
        static_cast<float>(character.lifespan);

    const float healthFactor =
        1.0f /
        (static_cast<float>(character.health) / 100.0f +
         healthOffset);

    const float probability =
        ageInfluence *
        std::pow(ageRatio, ageExponent) *
        healthFactor;

    return std::clamp(probability, 0.0f, 1.0f);
}

// ============================================================
//  6: DEATH
// ============================================================

inline void KillCharacter(
    Character& character,
    RecordStore<MarriageRelation>& marriages,
    const WorldTime& date)
{
    if (!character.alive)
        return;

    character.alive = false;
    character.deathDate = date;

    ProcessWidowhood(marriages, character, date);

    for (auto& [id, marriage] : marriages) {
        if (marriage.status != MarriageStatus::Betrothed)
            continue;

        if (marriage.chara_a.id == character.id ||
            marriage.chara_b.id == character.id) {
            marriage.status = MarriageStatus::BetrothalBroken;
            marriage.endDate = date;
        }
    }
}
inline void ProcessDeaths(RecordStore<Character>& characters,RecordStore<MarriageRelation>&marriage_relations,WorldTime&date)
    {
        for (auto& [id, character] : characters)
        {
            if (!character.alive)
                continue;

            float probability = GetCharaDeathProbability(
                character
            );

            float roll = GetRandomFloatValue(0.0f, 1.0f);

            if (roll < probability)
                KillCharacter(character,marriage_relations,date);
        }
    }
// ============================================================
//  7: PREGNANCY
// ============================================================

inline bool CanConcieve(
    const json& settings,
    const RecordStore<Pregnancy>& pregnancies,
    const Character& character)
{
    if (!character.alive || character.gender != 1)
        return false;

    const int conceiveAge =
        settings.at("conceive-age").get<int>();

    if (character.age < conceiveAge)
        return false;

    return !pregnancies.Any(
        [&](const Pregnancy& pregnancy) {
            return pregnancy.mother.id == character.id &&
                   pregnancy.status == PregnancyStatus::Developing;
        });
}

inline bool CreatePregnancy(
    const json& settings,
    RecordStore<Pregnancy>& pregnancies,
    const RecordStore<Character>& characters,
    const Character& mother,
    const Character& father,
    const WorldTime& conceptionDate,
    int numberOfChildren = 1)
{
    if (!CanConcieve(settings, pregnancies, mother))
        return false;

    if (mother.id == father.id ||
        mother.gender != 1 ||
        father.gender != 0) {
        return false;
    }

    if (!father.alive)
        return false;

    if (numberOfChildren < 1)
        return false;

    if (!characters.Contains(father.id))
        return false;

    Pregnancy pregnancy;

    pregnancy.mother = {mother.id};
    pregnancy.father = {father.id};
    pregnancy.conceptionDate = conceptionDate;
    pregnancy.pregnancyDuration = 10;
    pregnancy.children = numberOfChildren;
    pregnancy.status = PregnancyStatus::Developing;

    pregnancies.Insert(pregnancy);

    return true;
}

// Returns the expected birth date of a pregnancy.
inline WorldTime GetPregnancyBirthDate(
    const Pregnancy& pregnancy)
{
    return AddMonths(
        pregnancy.conceptionDate,
        pregnancy.pregnancyDuration
    );
}

inline void ProcessPregnancies(
    RecordStore<Pregnancy>& pregnancies,
    RecordStore<Character>& characters,
    RecordStore<FamilyRelation>& familyRelations,
    const json& names,
    const WorldTime& currentDate)
{
    for (auto& [id, pregnancy] : pregnancies) {
        if (pregnancy.status != PregnancyStatus::Developing)
            continue;

        Character* mother =
            characters.Get(pregnancy.mother.id);

        Character* father =
            characters.Get(pregnancy.father.id);

        // Missing mother or father record is invalid.
        if (!mother || !father) {
            pregnancy.status = PregnancyStatus::Miscarriage;
            continue;
        }

        // Mother's death causes miscarriage.
        // Father's death does not.
        if (!mother->alive) {
            pregnancy.status = PregnancyStatus::Miscarriage;
            continue;
        }

        const WorldTime birthDate =
            GetPregnancyBirthDate(pregnancy);

        // Pregnancy has not reached its birth date yet.
        if (IsDateBefore(currentDate, birthDate))
            continue;

        // Create the expected children.
        for (int i = 0; i < pregnancy.children; ++i) {
            CreateChildBetween(
                names,
                familyRelations,
                characters,
                *mother,
                *father,
                pregnancy.conceptionDate,
                birthDate
            );
        }

        pregnancy.status = PregnancyStatus::Birthed;
    }
}

inline void MiscarryPregnancy(
    RecordStore<Pregnancy>& pregnancies,
    Uuid pregnancyId)
{
    Pregnancy* pregnancy =
        pregnancies.Get(pregnancyId);

    if (pregnancy &&
        pregnancy->status == PregnancyStatus::Developing) {
        pregnancy->status = PregnancyStatus::Miscarriage;
    }
}