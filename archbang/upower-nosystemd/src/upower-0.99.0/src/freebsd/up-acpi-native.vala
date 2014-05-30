public class UpAcpiNative : Object {
    private string? _driver;
    private int _unit;
    private string _path;

    public string driver {
	get { return _driver; }
    }

    public int unit {
	get { return _unit; }
    }

    public string path {
	get { return _path; }
    }

    public UpAcpiNative (string path) {
	Regex r;
	MatchInfo mi;
	bool ret;

	try {
	    r = new Regex("dev\\.([^\\.])\\.(\\d+)");
	    ret = r.match(path, 0, out mi);
	    if (ret) {
	        _driver = mi.fetch(1);
	        _unit = mi.fetch(2).to_int();
	    } else {
	        _driver = null;
	        _unit = -1;
	    }
	} catch (RegexError re) {
	    _driver = null;
	    _unit = -1;
	}

	_path = path;
    }

    public UpAcpiNative.driver_unit (string driver, int unit) {
	_driver = driver;
	_unit = unit;
	_path = "dev.%s.%i".printf (_driver, _unit);
    }
}
