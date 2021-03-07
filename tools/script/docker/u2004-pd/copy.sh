id=$(docker create u2004-pd)
# docker cp $id:/ - > ./pd.tar.gz
docker cp $id:/pd_build.log - > ./pd_build.log
docker rm -v $id
