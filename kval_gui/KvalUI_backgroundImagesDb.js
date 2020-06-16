
var _movieBackground = [];

var _serieBackground = ["http://thetvdb.com/banners/fanart/original/73762-28.jpg",
                        "http://thetvdb.com/banners/fanart/original/80547-24.jpg",
                        "http://thetvdb.com/banners/fanart/original/73800-43.jpg",
                        "http://thetvdb.com/banners/fanart/original/80379-38.jpg",
                        "http://thetvdb.com/banners/fanart/original/121361-15.jpg"];

var _mangasBackground =["http://thetvdb.com/banners/fanart/original/81797-27.jpg",
                        "http://thetvdb.com/banners/fanart/original/79824-36.jpg",
                        "http://thetvdb.com/banners/fanart/original/114801-9.jpg",
                        "http://thetvdb.com/banners/fanart/original/88031-2.jpg",
                        "http://thetvdb.com/banners/fanart/original/79481-6.jpg"];
var isPreFilled = 0;

function getBackgroundRandom()
{return Math.floor((Math.random() * 13) + 1);
}


function preFillBackGroundArt(template)
{
    if(isPreFilled) return;
    for(var i = 1; i<14 ; i++)
    {_movieBackground.push(template+i+".jpg")
    }
    isPreFilled = 1
}

function storeBackgroundImage(imageCategory, imagePath)
{
    if(imageCategory === "movie")
    {_movieBackground.push(imagePath);
    }
    else if(imageCategory === "serie")
    {_serieBackground.push(imagePath);
    }
    else if(imageCategory === "mangas")
    {_mangasBackground.push(imagePath);
    }
    else
    {console.log("Unknown image category !!")
    }
}


function getRandomBackgroundImage(imageCategory)
{
    if(imageCategory === "movie")
    {
        var x = Math.floor((Math.random() * _movieBackground.length));
        return _movieBackground[x];
    }
    else if(imageCategory === "serie")
    {
        var y = Math.floor((Math.random() * _serieBackground.length));
        return _serieBackground[y];
    }
    else if(imageCategory === "mangas")
    {
        var z = Math.floor((Math.random() * _mangasBackground.length));
        return _mangasBackground[z];
    }
    else
    {console.log("Unknown image category !!")
    }
}
