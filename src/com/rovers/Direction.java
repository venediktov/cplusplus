package com.rovers;

public enum Direction {
    N { //North
        XY face(final XY rover) {
            return new XY(rover.x,rover.y+1);
        }
    },
    W { //West
        XY face(final XY rover) {
            return new XY(rover.x-1,rover.y);
        }
    },
    S { //South
        XY face(final XY rover) {
            return new XY(rover.x,rover.y-1);
        }
    },
    E {
        XY face(final XY rover) {
            return new XY(rover.x+1,rover.y);
        }
    };
    XY face (final XY rover) {
        return new XY(rover.x,rover.y);
    }
}
