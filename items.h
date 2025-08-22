#ifndef ITEMS_H
#define ITEMS_H

#define ITEM_POTION 0
#define ITEM_WEAPON 1
#define ITEM_ARMOR 2

typedef struct Item {
    char name[50];
    int type;
    void (*use)(struct Item* self);
} Item;

typedef struct {
    Item base;
    int damage;
    int durability;
} Weapon;

typedef struct {
    Item base;
    int defense;
    int durability;
} Armor;

typedef struct {
    Item base;
    int healAmount;
} Potion;

void useWeapon(Item* self);
void useArmor(Item* self);
void usePotion(Item* self);

#endif
