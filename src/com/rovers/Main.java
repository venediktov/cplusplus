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
        //if ( !in.hasNext(Pattern.compile("\\d\\s\\d\\n"))) {
        //    System.err.println("format of plot is wrong");
        //    System.exit(-1);
        //}
        Main.Xmax = in.nextInt();
        in.skip(" ");
        Main.Ymax = in.nextInt();
        while (in.hasNext()) {
            String[] roverSpec = in.next("\\d\\s\\d\\s[A-Z]").split("\\s+");
            List<Character> moves = in.next().chars().mapToObj(o->(char)o).collect(Collectors.toList());
            XY xy = new XY(Integer.parseInt(roverSpec[0]), Integer.parseInt(roverSpec[1]));
            Direction d = Direction.valueOf(roverSpec[3]);
            Rover rover = new Rover(xy,d);
            moves.stream().forEach(m->
            {
                Moves.valueOf(m.toString()).move(rover);
            });
            System.out.println(rover.impl.x + " " + rover.impl.y + rover.direction());
        }
    }
}
