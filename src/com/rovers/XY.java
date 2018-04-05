package com.rovers;

public class XY {
    public int x = 0;
    public int y = 0;
    public XY(int x , int y) {
        this.x = x ;
        this.y = y;
    }
    Direction direction(final XY other) {
        if ( x == other.x ) {
            if ( y < other.y) {
                return Direction.N;
            } else {
                return Direction.S;
            }
        }

        if( y == other.y) {
            if(x < other.x ) {
                return Direction.E;
            } else {
                return Direction.W;
            }
        }

        throw new RuntimeException("Direction is mesed up");
    }
}
