package com.rovers;


public enum Moves {
    L{ //Left
        @Override
        public void move(final Rover rover) {
            switch(rover.direction()) {
                case N:
                    rover.direction(Direction.W);
                    break;
                case W:
                    rover.direction(Direction.S);
                    break;
                case S:
                    rover.direction(Direction.E);
                    break;
                case E:
                    rover.direction(Direction.N);
                    break;
            }
        }
    },
    R{ //Right
        @Override
        public void move(final Rover rover) {
            switch(rover.direction()) {
                case N:
                    rover.direction(Direction.E);
                    break;
                case E:
                    rover.direction(Direction.S);
                    break;
                case S:
                    rover.direction(Direction.W);
                    break;
                case W:
                    rover.direction(Direction.N);
                    break;
            }
        }
    },
    M{ //Forward
        @Override
        public void move(final Rover rover) {
            switch(rover.direction()) {
                case N:
                    rover.impl.y = Math.max(Main.Ymin, Math.min(Main.Ymax,rover.face.y)); //extra guard Math.max
                    rover.face.y += 1;
                    break;
                case S:
                    rover.impl.y = Math.min(Main.Ymax, Math.max(Main.Ymin,rover.face.y)); //extra guard Math.min
                    rover.face.y -=1;
                    break;
                case E:
                    rover.impl.x = Math.max(Main.Xmin, Math.min(Main.Xmax,rover.face.x)); //extra guard Math.max
                    rover.face.x += 1;
                    break;
                case W:
                    rover.impl.x = Math.min(Main.Xmax, Math.max(Main.Xmin,rover.face.x)); //extra guard Math.min
                    rover.face.x -= 1;
                    break;
            }
        }
    };
    public void move(final Rover rover) {

    }
}

