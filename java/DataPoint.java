import java.util.Date;

class DataPoint {
	private Date time;
	private double value;
	private double min;
	private double max;
	
	private DataPoint() { }

	public DataPoint(Date time, double value, double min, double max) {
		setTime(time);
		setValue(value);
		setMin(min);
		setMax(max);
	}

	public void setTime(Date time) {
		this.time = time;
	}
	public void setValue(double value) {
		this.value = value;
	}
	public void setMin(double min) {
		this.min = min;
	}
	public void setMax(double max) {
		this.max = max;
	}
	public Date getTime() {
		return time;
	}
	public double getValue() {
		return value;
	}
	public double getMin() {
		return min;
	}
	public double getMax() {
		return max;
	}
}
