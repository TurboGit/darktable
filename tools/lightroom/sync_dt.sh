#! /bin/sh

DT=$HOME/.config/darktable
DTLIB=$DT/library.db

if [ ! -f $DT/lr.db ]; then
    echo please, create lr.db using get_lr_data.sh first
    exit 1
fi

# select only images with flags=1 (this is the default value when imported)

cat<<EOF | sqlite3 $DTLIB > /tmp/data
select images.id, folder || '/' || filename from images, film_rolls where flags&7=1 and film_rolls.id=images.film_id;
CREATE UNIQUE INDEX IF NOT EXISTS color_labels_idx ON color_labels(imgid,color);
EOF

cat<<EOF | sqlite3 $DT/lr.db
DROP TABLE IF EXISTS files;

CREATE TABLE files (
   id INTEGER,
   dt_filename VARCHAR);

.import /tmp/data files
EOF

inject_data()
{
    echo "$1;" >> $DT/lrsync.log
    echo "$1;" | sqlite3 $DTLIB
}

parse_data()
{
    id=$1
    rating=$2
    pick=$3
    color=$4
    gpslat=$5
    gpslon=$6
    hasgps=$7

    if [ $pick = 1 ]; then
        if [ $rating != "''" ]; then
            inject_data "UPDATE images SET flags=flags|$rating WHERE images.id=$id"
        fi

    elif [ $pick = -1 ]; then
        # rejected
        inject_data "UPDATE images SET flags=flags|6 WHERE images.id=$id"

    else
        # pick=0 (not selected), set to 0 star
        inject_data "UPDATE images SET flags=flags&~7 WHERE images.id=$id"
    fi

    case "$color" in
        Rouge|Red)
            inject_data "INSERT OR IGNORE INTO color_labels VALUES ($id,0)"
            ;;
        Jaune|Yellow)
            inject_data "INSERT OR IGNORE INTO color_labels VALUES ($id,1)"
            ;;
        Vert|Green)
            inject_data "INSERT OR IGNORE INTO color_labels VALUES ($id,2)"
            ;;
        Bleu|Blue)
            inject_data "INSERT OR IGNORE INTO color_labels VALUES ($id,3)"
            ;;
        \'\')
            ;;
        *)
            inject_data "INSERT OR IGNORE INTO color_labels VALUES ($id,4)"
            ;;
    esac

    if [ $hasgps != 0 ]; then
        inject_data "UPDATE images SET longitude=$gpslon, latitude=$gpslat WHERE images.id=$id"
    fi
}

get_kw_id()
{
    kwid=0

    echo "SELECT id FROM tags WHERE name=\"$1\";" | sqlite3 $DTLIB | while read id; do echo $id; done;
}

is_attached()
{
    id=$1
    kwid=$2

    echo "SELECT imgid FROM tagged_images WHERE imgid=$1 AND tagid=$kwid;" | sqlite3 $DTLIB | while read id; do echo $id; done;
}

parse_kw()
{
    id=$1
    kw="$2"

    # create tag if needed

    kwid=$(get_kw_id "$2")

    if [ -z $kwid ]; then
        echo New tag: $2
        inject_data "INSERT OR IGNORE INTO tags (id,name) VALUES ((SELECT id FROM tags WHERE name=\"$2\"),\"$2\")"
        inject_data "INSERT INTO tagxtag SELECT id, (SELECT id FROM tags WHERE name=\"$2\"), 0 FROM tags"
        inject_data "UPDATE tagxtag SET count = 1000000 WHERE id1 = (SELECT id FROM tags WHERE name=\"$2\") AND id2 = (SELECT id FROM tags WHERE name=\"$2\")"
        kwid=$(get_kw_id "$2")
    fi

    # attach tag

    if [ -z $(is_attached $id $kwid) ]; then
        inject_data "INSERT OR REPLACE INTO tagged_images (imgid, tagid) VALUES ($id, $kwid)"
        inject_data "UPDATE tagxtag SET count = count + 1 WHERE (id1 = $kwid AND id2 IN (SELECT tagid FROM tagged_images WHERE imgid = $id)) OR (id2 = $kwid AND id1 IN (SELECT tagid FROM tagged_images WHERE imgid = $id))"
    fi
}

cat<<EOF | sqlite3 -separator ' ' $DT/lr.db | while read id rating pick color gpslat gpslon hgps; do parse_data $id "$rating" "$pick" "$color" "$gpslat" "$gpslon" "$hgps"; done
SELECT id, QUOTE(rating), pick, QUOTE(colorLabels), QUOTE(gpsLatitude), QUOTE(gpsLongitude), hasGPS FROM data, files WHERE files.dt_filename like '%'||data.filename;
EOF

cat<<EOF | sqlite3 -separator ' ' $DT/lr.db | while read id kw; do parse_kw $id "$kw"; done
SELECT id, keyword FROM keywords, files WHERE files.dt_filename like '%'||keywords.filename;
EOF
