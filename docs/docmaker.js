// @jxarco

const keywords = ['int', 'float', 'double', 'bool', 'char', 'wchar_t', 'const', 'static_cast', 'dynamic_cast', 'new', 'delete', 'void', 'true', 'false', 'auto', 'struct', 'typedef', 'nullptr', 
    'NULL', 'unsigned', 'namespace'];

const flow_control = ['for', 'if', 'else', 'return', 'continue', 'break', 'case', 'switch', 'while', 'using'];

function MAKE_LINE_BREAK()
{
    document.body.appendChild( document.createElement('br') );
}

function MAKE_HEADER( string, type, id )
{
    console.assert(string && type);
    let header = document.createElement(type);
    header.innerHTML = string;
    if(id) header.id = id;
    document.body.appendChild( header );
}

function MAKE_PARAGRAPH( string, sup )
{
    console.assert(string);
    let paragraph = document.createElement(sup ? 'sup' : 'p');
    paragraph.innerHTML = string;
    document.body.appendChild( paragraph );
}

function MAKE_CODE( string )
{
    console.assert(string);

    string.replaceAll('<', '&lt;');
    string.replaceAll('>', '&gt;');

    let highlight = "";
    let content = "";

    const getHTML = ( h, c ) => {
        return `<span class="${ h }">${ c }</span>`;
    };

    for( let i = 0; i < string.length; ++i )
    {
        let char = string[ i ];

        if( char == '@' )
        {
            const str = string.substr( i + 1 );
            let html = null;

            // Highlight is specified
            if( string[ i + 1] == '[' )
            {
                highlight = str.substr( 1, 3 );
                content = str.substring( 5, str.indexOf( '@' ) );
                html = getHTML( highlight, content );
                string = string.replace( `@[${ highlight }]${ content }@`, html );
            }
            else
            {
                content = str.substring( 0, str.indexOf( '@' ) );

                if( keywords.includes( content ) )
                {
                    highlight = "kwd";    
                }
                else if( flow_control.includes( content ) )
                {
                    highlight = "flw";    
                }
                else
                {
                    console.error( "ERROR[Code Parsing]: Unknown highlight type: " + content );
                    return;
                }

                html = getHTML( highlight, content );
                string = string.replace( `@${ content }@`, html );
            }

            i += ( html.length - 1 );
        }
    }
    
    let pre = document.createElement('pre');
    let code = document.createElement('code');
    code.innerHTML = string;

    let button = document.createElement('button');
    button.title = "Copy";
    button.innerHTML = `<i class="fa-regular fa-copy"></i>`;
    button.addEventListener('click', COPY_SNIPPET.bind(this, button));

    code.appendChild( button );
    pre.appendChild( code );
    document.body.appendChild( pre );
}

function MAKE_BULLET_LIST( list )
{
    console.assert(list && list.length > 0);
    let ul = document.createElement('ul');
    for( var el of list ) {
        let li = document.createElement('li');
        li.innerHTML = el;
        ul.appendChild( li );
    }
    document.body.appendChild( ul );
}

function MAKE_CODE_BULLET_LIST( list, target )
{
    console.assert(list && list.length > 0);
    let ul = document.createElement('ul');
    for( var el of list ) {
        let split = (el.constructor === Array);
        if( split && el[0].constructor === Array ) {
            MAKE_CODE_BULLET_LIST( el, ul );
            continue;
        }
        let li = document.createElement('li');
        li.innerHTML = split ? INLINE_CODE( el[0] ) + ": " + el[1] : INLINE_CODE( el );
        ul.appendChild( li );
    }
    if( target ) {
        target.appendChild( ul );
    } else {
        document.body.appendChild( ul );
    }
}

function INLINE_LINK( string, href )
{
    console.assert(string && href);
    return `<a href="` + href + `">` + string + `</a>`;
}

function INLINE_CODE( string )
{
    console.assert(string);
    return `<code class="inline">` + string + `</code>`;
}

function COPY_SNIPPET( b )
{
    b.innerHTML = '<i class="fa-solid fa-check"></i>';
    b.classList.add('copied');

    setTimeout( () => {
        b.innerHTML = '<i class="fa-regular fa-copy"></i>';
        b.classList.remove('copied');
    }, 2000 );

    navigator.clipboard.writeText( b.dataset['snippet'] ?? b.parentElement.innerText );
    console.log("Copied!");
}

function INSERT_IMAGE( src, caption = "" )
{
    let img = document.createElement('img');
    img.src = src;
    img.alt = caption;
    document.body.appendChild( img );
}