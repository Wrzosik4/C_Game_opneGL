#include <stdio.h>
#include <string.h>
#include "items.h"

void useWeapon(Item* self) {
    Weapon* weapon = (Weapon*)self;

    if (weapon->durability > 0) {
        printf("Używasz %s! Obrażenia: %d\n", w->base.name, w->damage);
        weapon->durability--;
        printf("Wytrzymałość broni: %d\n", w->durability);
    } else {
        printf("%s jest zniszczona!\n", w->base.name);
    }
}

void useArmor(Item* self) {
    Armor* armor = (Armor*)self;

    if (armor->durability > 0) {
        printf("Używasz %s! Obrona: %d\n", w->base.name, w->defence);
        armor->durability--;
        printf("Wytrzymałość broni: %d\n", w->durability);
    } else {
        printf("%s jest zniszczona!\n", w->base.name);
    }
}

void usePotion(Item* self) {
    Potion* potion = (Potion*)self;
    printf("Używasz %s! Leczenie: %d\n", potion->base.name, potion->healAmount);
}