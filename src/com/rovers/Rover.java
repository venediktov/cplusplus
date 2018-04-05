package com.rovers;

public class Rover {
    XY impl;
    Direction direction;

    public Rover(final XY other, final Direction d) {
        impl = other;
        direction = d ;
    }
    Direction direction() {
        return direction;
    }

    void direction(final Direction d) {
        direction = d;
    }
}
