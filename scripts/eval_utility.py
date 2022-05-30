import re
import gzip
import json
import os
import numpy as np

# https://stackoverflow.com/a/46801075/6669161
def slugify(s):
    if not isinstance(s, str):
        s = " ".join(str(x) for x in s)
    s = str(s).strip().replace(' ', '_')
    return re.sub(r'(?u)[^-\w.]', '', s)

def load_env_config():
    import dotenv
    return {
        **dotenv.dotenv_values(".env"),  # load shared development variables
        **dotenv.dotenv_values(".env.local"),  # load sensitive variables
        **os.environ,  # override loaded values with environment variables
    }

# Source: https://github.com/mpld3/mpld3/issues/434#issuecomment-340255689
class NumpyEncoder(json.JSONEncoder):
    """ Special json encoder for numpy types """
    def default(self, obj):
        if isinstance(obj, (np.int_, np.intc, np.intp, np.int8,
                            np.int16, np.int32, np.int64, np.uint8,
                            np.uint16, np.uint32, np.uint64)):
            return int(obj)
        elif isinstance(obj, (np.float_, np.float16, np.float32,
                              np.float64)):
            return float(obj)
        elif isinstance(obj,(np.ndarray,)):
            return obj.tolist()
        return json.JSONEncoder.default(self, obj)


export_cache_dir = None

def cached(id, proc_cb=None):
    if export_cache_dir:
        cache_file = os.path.join(export_cache_dir, slugify(id) + '.json.gz')
        if not os.path.isfile(cache_file):
            data = proc_cb()
            with gzip.open(cache_file, 'wt', encoding='UTF-8') as zipfile:
                json.dump(data, zipfile, cls=NumpyEncoder)
        with gzip.open(cache_file, 'rt', encoding='UTF-8') as json_file:
            return json.load(json_file)
    else:
        return proc_cb()

def init_cache(path):
    global export_cache_dir
    export_cache_dir = path
    os.makedirs(export_cache_dir, exist_ok=True)