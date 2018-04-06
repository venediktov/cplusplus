package com.rovers;

public class Rover {
    private XY coordinates;
    private Direction direction;

    public Rover(final XY other, final Direction d) {
        coordinates = other;
        direction = d ;
    }
    Direction direction() {
        return direction;
    }
    XY xy() {
        return coordinates;
    }
    void turn(final Direction d) {
        direction = d;
    }
    void moveX(int x) {
        coordinates.x = x;
    }
    void moveY(int y) {
        coordinates.y = y;
    }
}
