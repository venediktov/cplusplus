package com.rovers;
import java.util.List;
import java.util.Scanner;
import java.util.stream.Collectors;


public class Main {

    static final int Ymin = 0;//according to assignment it's immutable
    static int Ymax = 0;
    static final int Xmin = 0; //acccording to assignment it's immmutble
    static int Xmax = 0;

    public static void main(String[] args) {
        Scanner in = new Scanner(System.in);
        Main.Xmax = in.nextInt();
        in.skip(" ");
        Main.Ymax = in.nextInt();
        if (in.hasNextLine()) { //skip line leftover from above
            in.nextLine();
        }
        while (in.hasNextLine()) {
            String tmp = in.nextLine();
            String[] roverSpec = tmp.split("\\s");
            List<Character> moves = in.next().chars().mapToObj(o->(char)o).collect(Collectors.toList());
            if(in.hasNextLine()) {
                in.nextLine(); //skip terminating newline was not consumed by simple next()
            }
            XY xy = new XY(Integer.parseInt(roverSpec[0]), Integer.parseInt(roverSpec[1]));
            Direction d = Direction.valueOf(roverSpec[2]);
            Rover rover = new Rover(xy,d);
            moves.stream().forEach(m->
            {
                Moves.valueOf(m.toString()).move(rover);
            });
            System.out.println(rover.impl.x + " " + rover.impl.y + " " + rover.direction());
        }
    }
}
