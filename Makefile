.ONESHELL: # enables `cd` inside a target to work normally https://stackoverflow.com/a/30590240/4692662

current_dir = $(realpath .)

module_name=pathological_module

module_src=src/pathological.c

build: nginx/objs/nginx nginx/objs/$(module_name).so

nginx/objs/nginx: config
	cd nginx
	./auto/configure --with-compat --add-dynamic-module=..
	make

config: config.in
	sed "s/@MODULE_NAME@/$(module_name)/g" config.in > config
	sed -i 's~@MODULE_SRC@~$(addprefix $$ngx_addon_dir/,$(module_src))~g' config

nginx/objs/$(module_name).so: $(module_src)
	cd nginx
	make modules

start: nginx/objs/nginx nginx/objs/$(module_name).so
	./nginx/objs/nginx -p nix -e stderr

clean:
	rm -rf nginx/objs && rm config

test: nginx/objs/nginx nginx/objs/$(module_name).so
	TEST_NGINX_GLOBALS="load_module $(current_dir)/nginx/objs/$(module_name).so;" TEST_NGINX_BINARY=../nginx/objs/nginx prove -I. -r t
