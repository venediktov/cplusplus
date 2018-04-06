package com.rovers;


public enum Moves {
    L { //Left
        @Override
        public void move(final Rover rover) {
            switch(rover.direction()) {
                case N:
                    rover.turn(Direction.W);
                    break;
                case W:
                    rover.turn(Direction.S);
                    break;
                case S:
                    rover.turn(Direction.E);
                    break;
                case E:
                    rover.turn(Direction.N);
                    break;
            }
        }
    },
    R { //Right
        @Override
        public void move(final Rover rover) {
            switch(rover.direction()) {
                case N:
                    rover.turn(Direction.E);
                    break;
                case E:
                    rover.turn(Direction.S);
                    break;
                case S:
                    rover.turn(Direction.W);
                    break;
                case W:
                    rover.turn(Direction.N);
                    break;
            }
        }
    },
    M { //Forward
        @Override
        public void move(final Rover rover) {
            Direction direction = rover.direction();
            XY face = direction.face(rover.xy());
            switch(direction) {
                case N:
                    rover.moveY(Math.max(Main.Ymin, Math.min(Main.Ymax,face.y))); //extra guard Math.max
                    break;
                case S:
                    rover.moveY(Math.min(Main.Ymax, Math.max(Main.Ymin,face.y))); //extra guard Math.min
                    break;
                case E:
                    rover.moveX(Math.max(Main.Xmin, Math.min(Main.Xmax,face.x))); //extra guard Math.max
                    break;
                case W:
                    rover.moveX(Math.min(Main.Xmax, Math.max(Main.Xmin,face.x))); //extra guard Math.min
                    break;
            }
        }
    };
    public void move(final Rover rover) {

    }
}

