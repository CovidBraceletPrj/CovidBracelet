Import("env", "projenv")

from os.path import isdir, join

try:
    import configparser
except ImportError:
    import ConfigParser as configparser

config = configparser.ConfigParser()
config.read("platformio.ini")

boot_hex_file = None

if 'PIOENV' in projenv:
    cur_env = "env:" + projenv['PIOENV']
    if config.has_section(cur_env) and config.has_option(cur_env, "boot_hex_file"):
        boot_hex_file = config.get(cur_env, "boot_hex_file")


# access to global build environment
print("Using boot hex file: " + str(boot_hex_file))

def _jlink_cmd_script_overwrite(env, source):
    build_dir = env.subst("$BUILD_DIR")
    if not isdir(build_dir):
        makedirs(build_dir)
    script_path = join(build_dir, "upload.jlink")
    
    commands = [ "h" ]
    commands.append("loadfile %s" % (boot_hex_file))
    commands.append("loadfile %s" % (source))
    commands.append("r")
    commands.append("q")

    with open(script_path, "w") as fp:
        fp.write("\n".join(commands))
    print(script_path)
    return script_path

def before_upload(source, target, env):    
    env.Replace(__jlink_cmd_script=_jlink_cmd_script_overwrite)

env.AddPreAction("upload", before_upload)