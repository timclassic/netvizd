import java.net.Socket;
import java.io.OutputStreamWriter;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.util.Date;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;

class NVConnector {

	private String host;
	private int port = 12346;
	private Socket sock;
	private BufferedReader br;
	private BufferedWriter bw;

	private NVConnector() { }

	public NVConnector(String host) throws IOException {
		setHost(host);
		connect();
	}

	public NVConnector(String host, int port) throws IOException {
		setHost(host);
		setPort(port);
		connect();
	}

	public void setHost(String host) {
		this.host = host;
	}

	public void setPort(int port) {
		this.port = port;
	}

	private void connect() throws IOException {
		String line;
		String[] words;

		sock = new Socket(host, port);
		br = new BufferedReader(new InputStreamReader(sock.getInputStream()));
		bw = new BufferedWriter(new OutputStreamWriter(sock.getOutputStream()));

		/* wait for proto version */
		for (;;) {
			line = br.readLine();
			if (line == null) {
				throw new IOException("Remote end disconnected.");
			}
			words = line.split(" ");
			if ("101".equals(words[0])) break;
			else if ("100".equals(words[0])) {
				/* do nothing, ident string */
			} else {
				throw new IOException("Invalid server header.");
			}
		}
	}

	public ArrayList getData(String system, String dset, Date start,
							 Date end, int resolution)
			throws IOException {
		String line;
		String[] words;

		/* get UNIX times - seconds since Jan 1, 1970 GMT */
		long start_secs = start.getTime() / 1000;
		long end_secs = end.getTime() / 1000;

		/* send request */
		String cmd = "FETCH " + system + " " + dset + " " +
				 String.valueOf(start_secs) + " " +
				 String.valueOf(end_secs) + " " + resolution + "\r\n";
		bw.write(cmd, 0, cmd.length());
		bw.flush();

		/* read back data */
		ArrayList list = new ArrayList();
		for (;;) {
			line = br.readLine();
			if (line == null) {
				throw new IOException("Remote end disconnected.");
			}
			words = line.split(" ");
			if ("103".equals(words[0])) {
				if (words.length != 5) {
					throw new IOException("Invalid server response for " +
										  "FETCH command.");
				}
				/* add to collection*/
				long utime = Long.parseLong(words[1]) * 1000;
				Date time = new Date(utime);
				double value = Double.parseDouble(words[2]);
				double min = Double.parseDouble(words[3]);
				double max = Double.parseDouble(words[4]);
				DataPoint dp = new DataPoint(time, value, min, max);
				list.add(dp);

			} else if ("104".equals(words[0])) {
				/* we're finished */
				cmd = "quit\r\n";
				bw.write(cmd, 0, cmd.length());
				break;
			} else if ("200".equals(words[0])) {
				throw new IOException("Server claims 'Invalid request.'");
			} else {
				throw new IOException("Invalid server response for FETCH " +
									  "command.");
			}
		}
		return list;
	}

	public static void main(String[] args) throws Exception {
		NVConnector nvc = new NVConnector("kami.stoo.org");

		DateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss z");
		Date start = df.parse("2005-04-27 00:00:00 EDT");
		Date end = df.parse("2005-04-28 12:00:00 EDT");

		ArrayList list = nvc.getData("stoo_rtr1", "atm_bytes_in", start,
									 end, 1);

		for (int i = 0; i < list.size(); i++) {
			DataPoint dp = (DataPoint)list.get(i);
			System.out.print("Time: " + dp.getTime() + "  ");
			System.out.print("Value: " + dp.getValue() + "  ");
			System.out.print("Min: " + dp.getMin() + "  ");
			System.out.print("Max: " + dp.getMax() + "\n");
		}
	}
}
