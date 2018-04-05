package com.rovers;

public class Rover {
    XY impl;
    XY face;
    public Rover(final XY other, final Direction d) {
        impl = other;
        face = d.face(other);
    }
    Direction direction() {
        return impl.direction(face);
    }

    void direction(final Direction d) {
        this.face = d.face(impl);
    }
}
