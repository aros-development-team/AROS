import java.net.*;
import java.io.*;

public class dec2utf8 {
    public static void main(String[] args) throws IOException {

        StringBuffer buf = new StringBuffer();

        for (int i = 0; i < args.length; i++) {
            String b = args[i];
            Character c = new Character ((char) Integer.parseInt(b));
            buf.append(c);
        }

        String out = buf.toString();
        byte outb[] = out.getBytes("UTF8");

        for (int i = 0; i < outb.length; i++) {
            System.out.print("0x"+Integer.toHexString(outb[i])+" ");
        }
    }
        

}
